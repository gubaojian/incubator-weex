/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <stdio.h>
#include "ecma_comparison.h"
#include "qking_core.h"
#include "debugger.h"
#include "ecma_alloc.h"
#include "ecma_array_object.h"
#include "ecma_arraybuffer_object.h"
#include "ecma_builtin_helpers.h"
#include "ecma_builtins.h"
#include "ecma_exceptions.h"
#include "ecma_function_object.h"
#include "ecma_gc.h"
#include "ecma_helpers.h"
#include "ecma_init_finalize.h"
#include "ecma_lex_env.h"
#include "ecma_literal_storage.h"
#include "ecma_objects.h"
#include "ecma_objects_general.h"
#include "ecma_promise_object.h"
#include "ecma_typedarray_object.h"
#include "jcontext.h"
#include "qking_debugger_transport.h"
#include "jmem.h"
#include "re_compiler.h"
#include "base/common_logger.h"
#include "core/vm/vm_ecma_value_helper.h"
#ifndef QKING_NDEBUG
#include "core/ecma/operations/ecma_compiled_code.h"
#endif

QKING_STATIC_ASSERT (sizeof (qking_value_t) == sizeof (ecma_value_t),
                     size_of_qking_value_t_must_be_equal_to_size_of_ecma_value_t);

QKING_STATIC_ASSERT ((int) ECMA_ERROR_NONE == (int) QKING_ERROR_NONE
                     && (int) ECMA_ERROR_COMMON == (int) QKING_ERROR_COMMON
                     && (int) ECMA_ERROR_RANGE == (int) QKING_ERROR_RANGE
                     && (int) ECMA_ERROR_REFERENCE == (int) QKING_ERROR_REFERENCE
                     && (int) ECMA_ERROR_SYNTAX == (int) QKING_ERROR_SYNTAX
                     && (int) ECMA_ERROR_TYPE == (int) QKING_ERROR_TYPE
                     && (int) ECMA_ERROR_URI == (int) QKING_ERROR_URI,
                     ecma_standard_error_t_must_be_equal_to_qking_error_t);

QKING_STATIC_ASSERT ((int) ECMA_INIT_EMPTY == (int) QKING_INIT_EMPTY
                     && (int) ECMA_INIT_SHOW_OPCODES == (int) QKING_INIT_SHOW_OPCODES
                     && (int) ECMA_INIT_SHOW_REGEXP_OPCODES == (int) QKING_INIT_SHOW_REGEXP_OPCODES
                     && (int) ECMA_INIT_MEM_STATS == (int) QKING_INIT_MEM_STATS,
                     ecma_init_flag_t_must_be_equal_to_qking_init_flag_t);

#if defined QKING_DISABLE_JS_PARSER && !defined QKING_ENABLE_SNAPSHOT_EXEC
#error QKING_ENABLE_SNAPSHOT_EXEC must be defined if QKING_DISABLE_JS_PARSER is defined!
#endif /* QKING_DISABLE_JS_PARSER && !QKING_ENABLE_SNAPSHOT_EXEC */

#ifdef QKING_ENABLE_ERROR_MESSAGES

/**
 * Error message, if an argument is has an error flag
 */
static const char * const error_value_msg_p = "argument cannot have an error flag";

/**
 * Error message, if types of arguments are incorrect
 */
static const char * const wrong_args_msg_p = "wrong type of argument";

#endif /* QKING_ENABLE_ERROR_MESSAGES */

/** \addtogroup qking Qking engine interface
 * @{
 */

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there are some states when API could not be invoked.
 *
 *         The API can be and only be invoked when the ECMA_STATUS_API_AVAILABLE
 *         flag is set.
 *
 *         This procedure checks whether the API is available, and terminates
 *         the engine if it is unavailable. Otherwise it is a no-op.
 *
 * Note:
 *         The API could not be invoked in the following cases:
 *           - before qking_init and after qking_cleanup
 *           - between enter to and return from a native free callback
 */
static inline void QKING_ATTR_ALWAYS_INLINE
qking_assert_api_available (void)
{
  QKING_ASSERT (QKING_CONTEXT (status_flags) & ECMA_STATUS_API_AVAILABLE);
} /* qking_assert_api_available */

/**
 * Turn on API availability
 */
static inline void QKING_ATTR_ALWAYS_INLINE
qking_make_api_available (void)
{
  QKING_CONTEXT (status_flags) |= ECMA_STATUS_API_AVAILABLE;
} /* qking_make_api_available */

/**
 * Turn off API availability
 */
static inline void QKING_ATTR_ALWAYS_INLINE
qking_make_api_unavailable (void)
{
  QKING_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_API_AVAILABLE;
} /* qking_make_api_unavailable */

/**
 * Create an API compatible return value.
 *
 * @return return value for Qking API functions
 */
static qking_value_t
qking_return (qking_value_t value) /**< return value */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    value = ecma_create_error_reference_from_context ();
  }

  return value;
} /* qking_return */

/**
 * Throw an API compatible return value.
 *
 * @return return value for Qking API functions
 */
static inline qking_value_t QKING_ATTR_ALWAYS_INLINE
qking_throw (qking_value_t value) /**< return value */
{
  QKING_ASSERT (ECMA_IS_VALUE_ERROR (value));
  return ecma_create_error_reference_from_context ();
} /* qking_throw */

/**
 * Qking engine initialization
 */
void
qking_init (qking_init_flag_t flags) /**< combination of Qking flags */
{
  /* This function cannot be called twice unless qking_cleanup is called. */
  QKING_ASSERT (!(QKING_CONTEXT (status_flags) & ECMA_STATUS_API_AVAILABLE));

  /* Zero out all non-external members. */
  memset (&QKING_CONTEXT (QKING_CONTEXT_FIRST_MEMBER), 0,
          sizeof (qking_context_t) - offsetof (qking_context_t, QKING_CONTEXT_FIRST_MEMBER));

  QKING_CONTEXT (qking_init_flags) = flags;

  qking_make_api_available ();

  jmem_init ();
  ecma_init ();
} /* qking_init */

/**
 * Terminate Qking engine
 */
void
qking_cleanup (void)
{
  qking_assert_api_available ();

#ifdef QKING_DEBUGGER
  if (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
  {
    qking_debugger_transport_close ();
  }
#endif /* QKING_DEBUGGER */

  for (qking_context_data_header_t *this_p = QKING_CONTEXT (context_data_p);
       this_p != NULL;
       this_p = this_p->next_p)
  {
    if (this_p->manager_p->deinit_cb)
    {
      this_p->manager_p->deinit_cb (QKING_CONTEXT_DATA_HEADER_USER_DATA (this_p));
    }
  }
    
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  ecma_free_all_enqueued_jobs ();
#endif /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
    
#ifndef QKING_NDEBUG
  ecma_finalize_compiled_function_state(QKING_CONTEXT(func_state_p));
#endif
    
  ecma_finalize ();
        
  qking_make_api_unavailable ();
    
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_info_leak_print();
#endif
    
  QKING_ASSERT(QKING_CONTEXT(ecma_gc_objects_number) == 0);

  for (qking_context_data_header_t *this_p = QKING_CONTEXT (context_data_p), *next_p = NULL;
       this_p != NULL;
       this_p = next_p)
  {
    next_p = this_p->next_p;
    if (this_p->manager_p->finalize_cb)
    {
      this_p->manager_p->finalize_cb (QKING_CONTEXT_DATA_HEADER_USER_DATA (this_p));
    }
    jmem_heap_free_block (this_p, sizeof (qking_context_data_header_t) + this_p->manager_p->bytes_needed);
  }
    
  jmem_finalize ();
    
} /* qking_cleanup */

/**
 * Retrieve a context data item, or create a new one.
 *
 * @param manager_p pointer to the manager whose context data item should be returned.
 *
 * @return a pointer to the user-provided context-specific data item for the given manager, creating such a pointer if
 * none was found.
 */
void *
qking_get_context_data (const qking_context_data_manager_t *manager_p)
{
  void *ret = NULL;
  qking_context_data_header_t *item_p;

  for (item_p = QKING_CONTEXT (context_data_p); item_p != NULL; item_p = item_p->next_p)
  {
    if (item_p->manager_p == manager_p)
    {
      return QKING_CONTEXT_DATA_HEADER_USER_DATA (item_p);
    }
  }

  item_p = jmem_heap_alloc_block (sizeof (qking_context_data_header_t) + manager_p->bytes_needed);
  item_p->manager_p = manager_p;
  item_p->next_p = QKING_CONTEXT (context_data_p);
  QKING_CONTEXT (context_data_p) = item_p;
  ret = QKING_CONTEXT_DATA_HEADER_USER_DATA (item_p);

  memset (ret, 0, manager_p->bytes_needed);
  if (manager_p->init_cb)
  {
    manager_p->init_cb (ret);
  }

  return ret;
} /* qking_get_context_data */

/**
 * Register external magic string array
 */
void
qking_register_magic_strings (const qking_char_t * const *ex_str_items_p, /**< character arrays, representing
                                                                           *   external magic strings' contents */
                              uint32_t count, /**< number of the strings */
                              const qking_length_t *str_lengths_p) /**< lengths of all strings */
{
  qking_assert_api_available ();

  lit_magic_strings_ex_set ((const lit_utf8_byte_t * const *) ex_str_items_p,
                            count,
                            (const lit_utf8_size_t *) str_lengths_p);
} /* qking_register_magic_strings */

/**
 * Run garbage collection
 */
void
qking_gc (qking_gc_mode_t mode) /**< operational mode */
{
  qking_assert_api_available ();

  ecma_gc_run (mode == QKING_GC_SEVERITY_LOW ? JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW
                                             : JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH);
} /* qking_gc */

/**
 * Get heap memory stats.
 *
 * @return true - get the heap stats successful
 *         false - otherwise. Usually it is because the MEM_STATS feature is not enabled.
 */
bool
qking_get_memory_stats (qking_heap_stats_t *out_stats_p) /**< [out] heap memory stats */
{
#ifdef JMEM_STATS
  if (out_stats_p == NULL)
  {
    return false;
  }

  jmem_heap_stats_t jmem_heap_stats;
  memset (&jmem_heap_stats, 0, sizeof (jmem_heap_stats));
  jmem_heap_get_stats (&jmem_heap_stats);

  *out_stats_p = (qking_heap_stats_t)
  {
    .version = 1,
    .size = jmem_heap_stats.size,
    .allocated_bytes = jmem_heap_stats.allocated_bytes,
    .peak_allocated_bytes = jmem_heap_stats.peak_allocated_bytes
  };

  return true;
#else
  QKING_UNUSED (out_stats_p);
  return false;
#endif
} /* qking_get_memory_stats */

/**
 * Run enqueued Promise jobs until the first thrown error or until all get executed.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return result of last executed job, may be error value.
 */
qking_value_t
qking_run_all_enqueued_jobs (void)
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  return ecma_process_all_enqueued_jobs ();
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  return ECMA_VALUE_UNDEFINED;
#endif /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* qking_run_all_enqueued_jobs */

/**
 * Get global object
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return api value of global object
 */
qking_value_t
qking_get_global_object (void)
{
  qking_assert_api_available ();

  return ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
} /* qking_get_global_object */

/**
 * Check if the specified value is an abort value.
 *
 * @return true  - if both the error and abort values are set,
 *         false - otherwise
 */
bool
qking_value_is_abort (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_error_reference (value))
  {
    return false;
  }

  ecma_error_reference_t *error_ref_p = ecma_get_error_reference_from_value (value);

  return (error_ref_p->refs_and_flags & ECMA_ERROR_REF_ABORT) != 0;
} /* qking_value_is_abort */

/**
 * Check if the specified value is an array object value.
 *
 * @return true  - if the specified value is an array object,
 *         false - otherwise
 */
bool
qking_value_is_array (const qking_value_t value) /**< qking api value */
{
  qking_assert_api_available ();

  return ecma_is_value_object_array(value);
} /* qking_value_is_array */

/**
 * Check if the specified value is boolean.
 *
 * @return true  - if the specified value is boolean,
 *         false - otherwise
 */
bool
qking_value_is_boolean (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_boolean (value);
} /* qking_value_is_boolean */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise
 */
bool
qking_value_is_constructor (const qking_value_t value) /**< qking api value */
{
  qking_assert_api_available ();

  return ecma_is_constructor (value);
} /* qking_value_is_constructor */

/**
 * Check if the specified value is an error or abort value.
 *
 * @return true  - if the specified value is an error value,
 *         false - otherwise
 */
bool
qking_value_is_error (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_error_reference (value);
} /* qking_value_is_error */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise
 */
bool
qking_value_is_function (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_op_is_callable (value);
} /* qking_value_is_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise
 */
bool
qking_value_is_number (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_number (value);
} /* qking_value_is_number */

/**
 * Check if the specified value is null.
 *
 * @return true  - if the specified value is null,
 *         false - otherwise
 */
bool
qking_value_is_null (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_null (value);
} /* qking_value_is_null */

/**
 * Check if the specified value is object.
 *
 * @return true  - if the specified value is object,
 *         false - otherwise
 */
bool
qking_value_is_object (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_object (value);
} /* qking_value_is_object */

/**
 * Check if the specified value is promise.
 *
 * @return true  - if the specified value is promise,
 *         false - otherwise
 */
bool
qking_value_is_promise (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  return (ecma_is_value_object (value)
          && ecma_is_promise (ecma_get_object_from_value (value)));
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  QKING_UNUSED (value);
  return false;
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* qking_value_is_promise */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise
 */
bool
qking_value_is_string (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_string (value);
} /* qking_value_is_string */

/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise
 */
bool
qking_value_is_undefined (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_undefined (value);
} /* qking_value_is_undefined */

/**
 * Check if the specified value is null or undefined.
 *
 * @return true  - if the specified value is null or undefined,
 *         false - otherwise
 */
bool
qking_value_is_null_or_undefined (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_null (value) || ecma_is_value_undefined (value);
} /* qking_value_is_null_or_undefined */


bool qking_value_equal(const qking_value_t left,const qking_value_t right){
  qking_assert_api_available ();

  return ecma_op_strict_equality_compare(left,right);
}

bool qking_value_strict_equal(const qking_value_t left,const qking_value_t right){
  qking_assert_api_available ();

  return qking_value_to_boolean(ecma_op_abstract_equality_compare(left,right));
}


/**
 * Perform the base type of the JavaScript value.
 *
 * @return qking_type_t value
 */
qking_type_t
qking_value_get_type (const qking_value_t value) /**< input value to check */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return QKING_TYPE_ERROR;
  }

  lit_magic_string_id_t lit_id = ecma_get_typeof_lit_id (value);

  QKING_ASSERT (lit_id != LIT_MAGIC_STRING__EMPTY);

  switch (lit_id)
  {
    case LIT_MAGIC_STRING_UNDEFINED:
    {
      return QKING_TYPE_UNDEFINED;
    }
    case LIT_MAGIC_STRING_BOOLEAN:
    {
      return QKING_TYPE_BOOLEAN;
    }
    case LIT_MAGIC_STRING_NUMBER:
    {
      return QKING_TYPE_NUMBER;
    }
    case LIT_MAGIC_STRING_STRING:
    {
      return QKING_TYPE_STRING;
    }
    case LIT_MAGIC_STRING_FUNCTION:
    {
      return QKING_TYPE_FUNCTION;
    }
    default:
    {
      QKING_ASSERT (lit_id == LIT_MAGIC_STRING_OBJECT);

      /* Based on the ECMA 262 5.1 standard the 'null' value is an object.
       * Thus we'll do an extra check for 'null' here.
       */
      return ecma_is_value_null (value) ? QKING_TYPE_NULL : QKING_TYPE_OBJECT;
    }
  }
} /* qking_value_get_type */

/**
 * Check if the specified feature is enabled.
 *
 * @return true  - if the specified feature is enabled,
 *         false - otherwise
 */
bool
qking_is_feature_enabled (const qking_feature_t feature) /**< feature to check */
{
  QKING_ASSERT (feature < QKING_FEATURE__COUNT);

  return (false
#ifdef QKING_ENABLE_ERROR_MESSAGES
          || feature == QKING_FEATURE_ERROR_MESSAGES
#endif /* QKING_ENABLE_ERROR_MESSAGES */
#ifndef QKING_DISABLE_JS_PARSER
          || feature == QKING_FEATURE_JS_PARSER
#endif /* !QKING_DISABLE_JS_PARSER */
#ifdef JMEM_STATS
          || feature == QKING_FEATURE_MEM_STATS
#endif /* JMEM_STATS */
#ifdef PARSER_DUMP_BYTE_CODE
          || feature == QKING_FEATURE_PARSER_DUMP
#endif /* PARSER_DUMP_BYTE_CODE */
#ifdef REGEXP_DUMP_BYTE_CODE
          || feature == QKING_FEATURE_REGEXP_DUMP
#endif /* REGEXP_DUMP_BYTE_CODE */
#ifdef QKING_ENABLE_SNAPSHOT_SAVE
          || feature == QKING_FEATURE_SNAPSHOT_SAVE
#endif /* QKING_ENABLE_SNAPSHOT_SAVE */
#ifdef QKING_ENABLE_SNAPSHOT_EXEC
          || feature == QKING_FEATURE_SNAPSHOT_EXEC
#endif /* QKING_ENABLE_SNAPSHOT_EXEC */
#ifdef QKING_DEBUGGER
          || feature == QKING_FEATURE_DEBUGGER
#endif /* QKING_DEBUGGER */
#ifdef QKING_VM_EXEC_STOP
          || feature == QKING_FEATURE_VM_EXEC_STOP
#endif /* QKING_VM_EXEC_STOP */
#ifndef CONFIG_DISABLE_JSON_BUILTIN
          || feature == QKING_FEATURE_JSON
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
          || feature == QKING_FEATURE_PROMISE
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
          || feature == QKING_FEATURE_TYPEDARRAY
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
#ifndef CONFIG_DISABLE_DATE_BUILTIN
          || feature == QKING_FEATURE_DATE
#endif /* !CONFIG_DISABLE_DATE_BUILTIN */
#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
          || feature == QKING_FEATURE_REGEXP
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
#ifdef QKING_ENABLE_LINE_INFO
          || feature == QKING_FEATURE_LINE_INFO
#endif /* QKING_ENABLE_LINE_INFO */
#ifdef QKING_ENABLE_LOGGING
          || feature == QKING_FEATURE_LOGGING
#endif /* QKING_ENABLE_LOGGING */
          );
} /* qking_is_feature_enabled */

/**
 * Create abort from an api value.
 *
 * Create abort value from an api value. If the second argument is true
 * it will release the input api value.
 *
 * @return api abort value
 */
qking_value_t
qking_create_abort_from_value (qking_value_t value, /**< api value */
                               bool release) /**< release api value */
{
  qking_assert_api_available ();

  if (QKING_UNLIKELY (ecma_is_value_error_reference (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (qking_value_is_abort (value))
    {
      return release ? value : qking_acquire_value (value);
    }

    value = qking_get_value_from_error (value, release);
    release = true;
  }

  if (!release)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_error_reference (value, false);
} /* qking_create_abort_from_value */

/**
 * Create error from an api value.
 *
 * Create error value from an api value. If the second argument is true
 * it will release the input api value.
 *
 * @return api error value
 */
qking_value_t
qking_create_error_from_value (qking_value_t value, /**< api value */
                               bool release) /**< release api value */
{
  qking_assert_api_available ();

  if (QKING_UNLIKELY (ecma_is_value_error_reference (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (!qking_value_is_abort (value))
    {
      return release ? value : qking_acquire_value (value);
    }

    value = qking_get_value_from_error (value, release);
    release = true;
  }

  if (!release)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_error_reference (value, true);
} /* qking_create_error_from_value */

/**
 * Get the value from an error value.
 *
 * Extract the api value from an error. If the second argument is true
 * it will release the input error value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return qking_value_t value
 */
qking_value_t
qking_get_value_from_error (qking_value_t value, /**< api value */
                            bool release) /**< release api value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_error_reference (value))
  {
    return release ? value : ecma_copy_value (value);
  }

  qking_value_t ret_val = qking_acquire_value (ecma_get_error_reference_from_value (value)->value);

  if (release)
  {
    qking_release_value (value);
  }
  return ret_val;
} /* qking_get_value_from_error */

/**
 * Return the type of the Error object if possible.
 *
 * @return one of the qking_error_t value as the type of the Error object
 *         QKING_ERROR_NONE - if the input value is not an Error object
 */
qking_error_t
qking_get_error_type (qking_value_t value) /**< api value */
{
  if (QKING_UNLIKELY (ecma_is_value_error_reference (value)))
  {
    value = ecma_get_error_reference_from_value (value)->value;
  }

  if (!ecma_is_value_object (value))
  {
    return QKING_ERROR_NONE;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);
  ecma_standard_error_t error_type = ecma_get_error_type (object_p);

  return (qking_error_t) error_type;
} /* qking_get_error_type */

/**
 * Get boolean from the specified value.
 *
 * @return true or false.
 */
bool
qking_get_boolean_value (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  return ecma_is_value_true (value);
} /* qking_get_boolean_value */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double
qking_get_number_value (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return (double) ecma_get_number_from_value (value);
} /* qking_get_number_value */

/**
 * Call ToBoolean operation on the api value.
 *
 * @return true  - if the logical value is true
 *         false - otherwise
 */
bool
qking_value_to_boolean (const qking_value_t value) /**< input value */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return false;
  }

  return ecma_op_to_boolean (value);
} /* qking_value_to_boolean */

/**
 * Call ToNumber operation on the api value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return converted number value - if success
 *         thrown error - otherwise
 */
qking_value_t
qking_value_to_number (const qking_value_t value) /**< input value */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return qking_return (ecma_op_to_number (value));
} /* qking_value_to_number */

/**
 * Call ToObject operation on the api value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return converted object value - if success
 *         thrown error - otherwise
 */
qking_value_t
qking_value_to_object (const qking_value_t value) /**< input value */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return qking_return (ecma_op_to_object (value));
} /* qking_value_to_object */

/**
 * Call ToPrimitive operation on the api value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return converted primitive value - if success
 *         thrown error - otherwise
 */
qking_value_t
qking_value_to_primitive (const qking_value_t value) /**< input value */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return qking_return (ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NO));
} /* qking_value_to_primitive */

/**
 * Call the ToString ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return converted string value - if success
 *         thrown error - otherwise
 */
qking_value_t
qking_value_to_string (const qking_value_t value) /**< input value */
{

  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return qking_return (ecma_op_to_string (value));
} /* qking_value_to_string */

/**
 * Acquire specified Qking API value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return acquired api value
 */
qking_value_t
qking_acquire_value (qking_value_t value) /**< API value */
{
  qking_assert_api_available ();

  if (QKING_UNLIKELY (ecma_is_value_error_reference (value)))
  {
    ecma_ref_error_reference (ecma_get_error_reference_from_value (value));
    return value;
  }

  return ecma_copy_value (value);
} /* qking_acquire_value */

/**
 * Release specified Qking API value
 */
void
qking_release_value (qking_value_t value) /**< API value */
{
  qking_assert_api_available ();

  if (QKING_UNLIKELY (ecma_is_value_error_reference (value)))
  {
    ecma_deref_error_reference (ecma_get_error_reference_from_value (value));
    return;
  }

  ecma_free_value (value);
} /* qking_release_value */

/**
 * Create an array object value
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the constructed array object
 */
qking_value_t
qking_create_array (uint32_t size) /**< size of array */
{
  qking_assert_api_available ();

  ecma_value_t array_length = ecma_make_uint32_value (size);

  const qking_length_t argument_size = 1;
  ecma_value_t array_value = ecma_op_create_array_object (&array_length, argument_size, true);
  ecma_free_value (array_length);

  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (array_value));

  return array_value;
} /* qking_create_array */

/**
 * Create a qking_value_t representing a boolean value from the given boolean parameter.
 *
 * @return value of the created boolean
 */
qking_value_t
qking_create_boolean (bool value) /**< bool value from which a qking_value_t will be created */
{
  qking_assert_api_available ();

  return qking_return (ecma_make_boolean_value (value));
} /* qking_create_boolean */

/**
 * Create an error object
 *
 * Note:
 *      - returned value must be freed with qking_release_value, when it is no longer needed
 *      - the error flag is set for the returned value
 *
 * @return value of the constructed error object
 */
qking_value_t
qking_create_error (qking_error_t error_type, /**< type of error */
                    const char *message_p) /**< value of 'message' property
                                                    *   of constructed error object */
{
  return qking_create_error_sz (error_type,
                                (lit_utf8_byte_t *) message_p,
                                lit_zt_utf8_string_size ((const lit_utf8_byte_t *)message_p));
} /* qking_create_error */

/**
 * Create an error object
 *
 * Note:
 *      - returned value must be freed with qking_release_value, when it is no longer needed
 *      - the error flag is set for the returned value
 *
 * @return value of the constructed error object
 */
qking_value_t
qking_create_error_sz (qking_error_t error_type, /**< type of error */
                       const qking_char_t *message_p, /**< value of 'message' property
                                                       *   of constructed error object */
                       qking_size_t message_size) /**< size of the message in bytes */
{
  qking_assert_api_available ();

  if (message_p == NULL || message_size == 0)
  {
    return ecma_create_error_object_reference (ecma_new_standard_error ((ecma_standard_error_t) error_type));
  }
  else
  {
    ecma_string_t *message_string_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) message_p,
                                                                      (lit_utf8_size_t) message_size);

    ecma_object_t *error_object_p = ecma_new_standard_error_with_message ((ecma_standard_error_t) error_type,
                                                                          message_string_p);

    ecma_deref_ecma_string (message_string_p);

    return ecma_create_error_object_reference (error_object_p);
  }
} /* qking_create_error_sz */

/**
 * Create an external function object
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the constructed function object
 */
qking_value_t
qking_create_external_function (qking_external_handler_t handler_p) /**< pointer to native handler
                                                                     *   for the function */
{
  qking_assert_api_available ();

  ecma_object_t *func_obj_p = ecma_op_create_external_function_object (handler_p);
  return ecma_make_object_value (func_obj_p);
} /* qking_create_external_function */

/**
 * Creates a qking_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return qking_value_t created from the given double argument.
 */
qking_value_t
qking_create_number (double value) /**< double value from which a qking_value_t will be created */
{
  qking_assert_api_available ();

  return ecma_make_number_value ((ecma_number_t) value);
} /* qking_create_number */

/**
 * Creates a qking_value_t representing a positive or negative infinity value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return qking_value_t representing an infinity value.
 */
qking_value_t
qking_create_number_infinity (bool sign) /**< true for negative Infinity
                                          *   false for positive Infinity */
{
  qking_assert_api_available ();

  return ecma_make_number_value (ecma_number_make_infinity (sign));
} /* qking_create_number_infinity */

/**
 * Creates a qking_value_t representing a not-a-number value.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return qking_value_t representing a not-a-number value.
 */
qking_value_t
qking_create_number_nan (void)
{
  qking_assert_api_available ();

  return ecma_make_nan_value ();
} /* qking_create_number_nan */

/**
 * Creates a qking_value_t representing an undefined value.
 *
 * @return value of undefined
 */
qking_value_t
qking_create_undefined (void)
{
  qking_assert_api_available ();

  return ECMA_VALUE_UNDEFINED;
} /* qking_create_undefined */

/**
 * Creates and returns a qking_value_t with type null object.
 *
 * @return qking_value_t representing null
 */
qking_value_t
qking_create_null (void)
{
  qking_assert_api_available ();

  return ECMA_VALUE_NULL;
} /* qking_create_null */

/**
 * Create new JavaScript object, like with new Object().
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the created object
 */
qking_value_t
qking_create_object (void)
{
  qking_assert_api_available ();

  return ecma_make_object_value (ecma_op_create_object_object_noarg ());
} /* qking_create_object */

/**
 * Create an empty Promise object which can be resolve/reject later
 * by calling qking_resolve_or_reject_promise.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the created object
 */
qking_value_t
qking_create_promise (void)
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  return ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_PROMISE_EXECUTOR_EMPTY);
#else /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Promise not supported.")));
#endif /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* qking_create_promise */

/**
 * Create string from a valid UTF-8 string
 *
 * Note:
 *      returned value must be freed with qking_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
qking_value_t
qking_create_string_from_utf8 (const qking_char_t *str_p) /**< pointer to string */
{
  return qking_create_string_sz_from_utf8 (str_p, lit_zt_utf8_string_size ((lit_utf8_byte_t *) str_p));
} /* qking_create_string_from_utf8 */

/**
 * Create string from a valid UTF-8 string
 *
 * Note:
 *      returned value must be freed with qking_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
qking_value_t
qking_create_string_sz_from_utf8 (const qking_char_t *str_p, /**< pointer to string */
                                  qking_size_t str_size) /**< string size */
{
  qking_assert_api_available ();

  ecma_string_t *ecma_str_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 ((lit_utf8_byte_t *) str_p,
                                                                                 (lit_utf8_size_t) str_size);

  return ecma_make_string_value (ecma_str_p);
} /* qking_create_string_sz_from_utf8 */

/**
 * Create string from a valid CESU-8 string
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the created string
 */
qking_value_t
qking_create_string (const qking_char_t *str_p) /**< pointer to string */
{
  return qking_create_string_sz (str_p, lit_zt_utf8_string_size ((lit_utf8_byte_t *) str_p));
} /* qking_create_string */

qking_value_t
qking_create_string_c (const char *str_p) /**< pointer to string */
{
  return qking_create_string((const qking_char_t *)str_p);
} /* qking_create_string_c */

qking_value_t
qking_create_string_lit (qking_magic_str_t name) /**< pointer to string */
{
  return ecma_make_magic_string_value(name);
} /* qking_create_string_c */

/**
 * Create string from a valid CESU-8 string
 *
 * Note:
 *      returned value must be freed with qking_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
qking_value_t
qking_create_string_sz (const qking_char_t *str_p, /**< pointer to string */
                        qking_size_t str_size) /**< string size */
{
  qking_assert_api_available ();

  ecma_string_t *ecma_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) str_p,
                                                              (lit_utf8_size_t) str_size);
  return ecma_make_string_value (ecma_str_p);
} /* qking_create_string_sz */

/**
 * Get length of an array object
 *
 * Note:
 *      Returns 0, if the value parameter is not an array object.
 *
 * @return length of the given array
 */
uint32_t
qking_get_array_length (const qking_value_t value) /**< api value */
{
  qking_assert_api_available ();

  if (!qking_value_is_array (value))
  {
    return 0;
  }

  ecma_value_t len_value = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (value),
                                                           LIT_MAGIC_STRING_LENGTH);

  qking_length_t length = ecma_number_to_uint32 (ecma_get_number_from_value (len_value));
  ecma_free_value (len_value);

  return length;
} /* qking_get_array_length */

/**
 * Get size of Qking string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of bytes in the buffer needed to represent the string
 */
qking_size_t
qking_get_string_size (const qking_value_t value) /**< input string */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_size (ecma_get_string_from_value (value));
} /* qking_get_string_size */

/**
 * Get UTF-8 encoded string size from Qking string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of bytes in the buffer needed to represent the UTF-8 encoded string
 */
qking_size_t
qking_get_utf8_string_size (const qking_value_t value) /**< input string */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_utf8_size (ecma_get_string_from_value (value));
} /* qking_get_utf8_string_size */

/**
 * Get length of Qking string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of characters in the string
 */
qking_length_t
qking_get_string_length (const qking_value_t value) /**< input string */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_length (ecma_get_string_from_value (value));
} /* qking_get_string_length */

/**
 * Get UTF-8 string length from Qking string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of characters in the string
 */
qking_length_t
qking_get_utf8_string_length (const qking_value_t value) /**< input string */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_utf8_length (ecma_get_string_from_value (value));
} /* qking_get_utf8_string_length */

/**
 * Copy the characters of a string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur in character buffer.
 *      Returns 0, if the value parameter is not a string or
 *      the buffer is not large enough for the whole string.
 *
 * Note:
 *      If the size of the string in qking value is larger than the size of the
 *      target buffer, the copy will fail.
 *      To copy substring use qking_substring_to_char_buffer() instead.
 *
 * @return number of bytes, actually copied to the buffer.
 */
qking_size_t
qking_string_to_char_buffer (const qking_value_t value, /**< input string value */
                             qking_char_t *buffer_p, /**< [out] output characters buffer */
                             qking_size_t buffer_size) /**< size of output buffer */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  if (ecma_string_get_size (str_p) > buffer_size)
  {
    return 0;
  }

  return ecma_string_copy_to_cesu8_buffer (str_p,
                                           (lit_utf8_byte_t *) buffer_p,
                                           buffer_size);
} /* qking_string_to_char_buffer */

/**
 * Copy the characters of an utf-8 encoded string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string or the buffer
 *      is not large enough for the whole string.
 *
 * Note:
 *      If the size of the string in qking value is larger than the size of the
 *      target buffer, the copy will fail.
 *      To copy a substring use qking_substring_to_utf8_char_buffer() instead.
 *
 * @return number of bytes copied to the buffer.
 */
qking_size_t
qking_string_to_utf8_char_buffer (const qking_value_t value, /**< input string value */
                                  qking_char_t *buffer_p, /**< [out] output characters buffer */
                                  qking_size_t buffer_size) /**< size of output buffer */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  if (ecma_string_get_utf8_size (str_p) > buffer_size)
  {
    return 0;
  }

  return ecma_string_copy_to_utf8_buffer (str_p,
                                          (lit_utf8_byte_t *) buffer_p,
                                          buffer_size);
} /* qking_string_to_utf8_char_buffer */

/**
 * Copy the characters of an cesu-8 encoded substring into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string.
 *      It will extract the substring beetween the specified start position
 *      and the end position (or the end of the string, whichever comes first).
 *
 * @return number of bytes copied to the buffer.
 */
qking_size_t
qking_substring_to_char_buffer (const qking_value_t value, /**< input string value */
                                qking_length_t start_pos, /**< position of the first character */
                                qking_length_t end_pos, /**< position of the last character */
                                qking_char_t *buffer_p, /**< [out] output characters buffer */
                                qking_size_t buffer_size) /**< size of output buffer */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  return ecma_substring_copy_to_cesu8_buffer (str_p,
                                              start_pos,
                                              end_pos,
                                              (lit_utf8_byte_t *) buffer_p,
                                              buffer_size);
} /* qking_substring_to_char_buffer */

/**
 * Copy the characters of an utf-8 encoded substring into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string.
 *      It will extract the substring beetween the specified start position
 *      and the end position (or the end of the string, whichever comes first).
 *
 * @return number of bytes copied to the buffer.
 */
qking_size_t
qking_substring_to_utf8_char_buffer (const qking_value_t value, /**< input string value */
                                     qking_length_t start_pos, /**< position of the first character */
                                     qking_length_t end_pos, /**< position of the last character */
                                     qking_char_t *buffer_p, /**< [out] output characters buffer */
                                     qking_size_t buffer_size) /**< size of output buffer */
{
  qking_assert_api_available ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  return ecma_substring_copy_to_utf8_buffer (str_p,
                                             start_pos,
                                             end_pos,
                                             (lit_utf8_byte_t *) buffer_p,
                                             buffer_size);
} /* qking_substring_to_utf8_char_buffer */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
qking_value_t
qking_has_property (const qking_value_t obj_val, /**< object value */
                    const qking_value_t prop_name_val) /**< property name (string value) */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return ecma_make_boolean_value (false);
  }

  bool has_property = ecma_op_object_has_property (ecma_get_object_from_value (obj_val),
                                                   ecma_get_string_from_value (prop_name_val));

  return ecma_make_boolean_value (has_property);
} /* qking_has_property */

/**
 * Checks whether the object has the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
qking_value_t
qking_has_own_property (const qking_value_t obj_val, /**< object value */
                        const qking_value_t prop_name_val) /**< property name (string value) */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return ecma_make_boolean_value (false);
  }

  bool has_property = ecma_op_object_has_own_property (ecma_get_object_from_value (obj_val),
                                                       ecma_get_string_from_value (prop_name_val));

  return ecma_make_boolean_value (has_property);
} /* qking_has_own_property */

/**
 * Delete a property from an object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
qking_delete_property (const qking_value_t obj_val, /**< object value */
                       const qking_value_t prop_name_val) /**< property name (string value) */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (obj_val),
                                                  ecma_get_string_from_value (prop_name_val),
                                                  false);
  return ecma_is_value_true (ret_value);
} /* qking_delete_property */

/**
 * Delete indexed property from the specified object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
qking_delete_property_by_index (const qking_value_t obj_val, /**< object value */
                                uint32_t index) /**< index to be written */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return false;
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (obj_val),
                                                  str_idx_p,
                                                  false);
  ecma_deref_ecma_string (str_idx_p);

  return ecma_is_value_true (ret_value);
} /* qking_delete_property_by_index */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_get_property (const qking_value_t obj_val, /**< object value */
                    const qking_value_t prop_name_val) /**< property name (string value) */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  qking_value_t ret_value = ecma_op_object_get (ecma_get_object_from_value (obj_val),
                                                ecma_get_string_from_value (prop_name_val));
  return qking_return (ret_value);
} /* qking_get_property */

/**
 * Get value by an index from the specified object.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return value of the property specified by the index - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_get_property_by_index (const qking_value_t obj_val, /**< object value */
                             uint32_t index) /**< index to be written */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_get (ecma_get_object_from_value (obj_val), str_idx_p);
  ecma_deref_ecma_string (str_idx_p);

  return qking_return (ret_value);
} /* qking_get_property_by_index */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_set_property (const qking_value_t obj_val, /**< object value */
                    const qking_value_t prop_name_val, /**< property name (string value) */
                    const qking_value_t value_to_set) /**< value to set */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value_to_set)
      || !ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  return qking_return (ecma_op_object_put (ecma_get_object_from_value (obj_val),
                                           ecma_get_string_from_value (prop_name_val),
                                           value_to_set,
                                           true));
} /* qking_set_property */

/**
 * Set indexed value in the specified object
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_set_property_by_index (const qking_value_t obj_val, /**< object value */
                             uint32_t index, /**< index to be written */
                             const qking_value_t value_to_set) /**< value to set */
{
  qking_assert_api_available ();

  if (ecma_is_value_error_reference (value_to_set)
      || !ecma_is_value_object (obj_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t ret_value = ecma_op_object_put (ecma_get_object_from_value (obj_val),
                                               str_idx_p,
                                               value_to_set,
                                               true);
  ecma_deref_ecma_string (str_idx_p);

  return qking_return (ret_value);
} /* qking_set_property_by_index */

/**
 * Initialize property descriptor.
 */
void
qking_init_property_descriptor_fields (qking_property_descriptor_t *prop_desc_p) /**< [out] property descriptor */
{
  prop_desc_p->is_value_defined = false;
  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->is_writable_defined = false;
  prop_desc_p->is_writable = false;
  prop_desc_p->is_enumerable_defined = false;
  prop_desc_p->is_enumerable = false;
  prop_desc_p->is_configurable_defined = false;
  prop_desc_p->is_configurable = false;
  prop_desc_p->is_get_defined = false;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->is_set_defined = false;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;
} /* qking_init_property_descriptor_fields */

/**
 * Define a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_define_own_property (const qking_value_t obj_val, /**< object value */
                           const qking_value_t prop_name_val, /**< property name (string value) */
                           const qking_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  if ((prop_desc_p->is_writable_defined || prop_desc_p->is_value_defined)
      && (prop_desc_p->is_get_defined || prop_desc_p->is_set_defined))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.is_enumerable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_enumerable_defined);
  prop_desc.is_enumerable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_enumerable_defined ? prop_desc_p->is_enumerable
                                                                                      : false);

  prop_desc.is_configurable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_configurable_defined);
  prop_desc.is_configurable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_configurable_defined ? prop_desc_p->is_configurable
                                                                                          : false);

  /* Copy data property info. */
  prop_desc.is_value_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_value_defined);

  if (prop_desc_p->is_value_defined)
  {
    if (ecma_is_value_error_reference (prop_desc_p->value))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    prop_desc.value = prop_desc_p->value;
  }

  prop_desc.is_writable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_writable_defined);
  prop_desc.is_writable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_writable_defined ? prop_desc_p->is_writable
                                                                                  : false);

  /* Copy accessor property info. */
  if (prop_desc_p->is_get_defined)
  {
    ecma_value_t getter = prop_desc_p->getter;
    prop_desc.is_get_defined = true;

    if (ecma_is_value_error_reference (getter))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    if (ecma_op_is_callable (getter))
    {
      prop_desc.get_p = ecma_get_object_from_value (getter);
    }
    else if (!ecma_is_value_null (getter))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }
  }

  if (prop_desc_p->is_set_defined)
  {
    ecma_value_t setter = prop_desc_p->setter;
    prop_desc.is_set_defined = true;

    if (ecma_is_value_error_reference (setter))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    if (ecma_op_is_callable (setter))
    {
      prop_desc.set_p = ecma_get_object_from_value (setter);
    }
    else if (!ecma_is_value_null (setter))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }
  }

  return ecma_op_object_define_own_property (ecma_get_object_from_value (obj_val),
                                             ecma_get_string_from_value (prop_name_val),
                                             &prop_desc,
                                             true);
} /* qking_define_own_property */

/**
 * Construct property descriptor from specified property.
 *
 * @return true - if success, the prop_desc_p fields contains the property info
 *         false - otherwise, the prop_desc_p is unchanged
 */
bool
qking_get_own_property_descriptor (const qking_value_t  obj_val, /**< object value */
                                   const qking_value_t prop_name_val, /**< property name (string value) */
                                   qking_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  ecma_property_descriptor_t prop_desc;

  if (!ecma_op_object_get_own_property_descriptor (ecma_get_object_from_value (obj_val),
                                                   ecma_get_string_from_value (prop_name_val),
                                                   &prop_desc))
  {
    return false;
  }

  prop_desc_p->is_configurable_defined = true;
  prop_desc_p->is_configurable = prop_desc.is_configurable;
  prop_desc_p->is_enumerable_defined = true;
  prop_desc_p->is_enumerable = prop_desc.is_enumerable;

  prop_desc_p->is_writable_defined = prop_desc.is_writable_defined;
  prop_desc_p->is_writable = prop_desc.is_writable_defined ? prop_desc.is_writable : false;

  prop_desc_p->is_value_defined = prop_desc.is_value_defined;
  prop_desc_p->is_get_defined = prop_desc.is_get_defined;
  prop_desc_p->is_set_defined = prop_desc.is_set_defined;

  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;

  if (prop_desc.is_value_defined)
  {
    prop_desc_p->value = prop_desc.value;
  }

  if (prop_desc.is_get_defined)
  {
    if (prop_desc.get_p != NULL)
    {
      prop_desc_p->getter = ecma_make_object_value (prop_desc.get_p);
    }
    else
    {
      prop_desc_p->getter = ECMA_VALUE_NULL;
    }
  }

  if (prop_desc.is_set_defined)
  {
    if (prop_desc.set_p != NULL)
    {
      prop_desc_p->setter = ecma_make_object_value (prop_desc.set_p);
    }
    else
    {
      prop_desc_p->setter = ECMA_VALUE_NULL;
    }
  }

  return true;
} /* qking_get_own_property_descriptor */

/**
 * Free fields of property descriptor (setter, getter and value).
 */
void
qking_free_property_descriptor_fields (const qking_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->is_value_defined)
  {
    qking_release_value (prop_desc_p->value);
  }

  if (prop_desc_p->is_get_defined)
  {
    qking_release_value (prop_desc_p->getter);
  }

  if (prop_desc_p->is_set_defined)
  {
    qking_release_value (prop_desc_p->setter);
  }
} /* qking_free_property_descriptor_fields */

/**
 * Invoke function specified by a function value
 *
 * Note:
 *      - returned value must be freed with qking_release_value, when it is no longer needed.
 *      - If function is invoked as constructor, it should support [[Construct]] method,
 *        otherwise, if function is simply called - it should support [[Call]] method.
 *
 * @return returned qking value of the invoked function
 */
static qking_value_t
qking_invoke_function (bool is_invoke_as_constructor, /**< true - invoke function as constructor
                                                       *          (this_arg_p should be NULL, as it is ignored),
                                                       *   false - perform function call */
                       const qking_value_t func_obj_val, /**< function object to call */
                       const qking_value_t this_val, /**< object value of 'this' binding */
                       const qking_value_t args_p[], /**< function's call arguments */
                       const qking_size_t args_count) /**< number of the arguments */
{
  QKING_ASSERT (args_count == 0 || args_p != NULL);

  if (ecma_is_value_error_reference (func_obj_val)
      || ecma_is_value_error_reference (this_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  for (uint32_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_error_reference (args_p[i]))
    {
      return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
    }
  }

  if (is_invoke_as_constructor)
  {
    QKING_ASSERT (qking_value_is_constructor (func_obj_val));

    return qking_return (ecma_op_function_construct (ecma_get_object_from_value (func_obj_val),
                                                     ECMA_VALUE_UNDEFINED,
                                                     args_p,
                                                     args_count));
  }
  else
  {
    QKING_ASSERT (qking_value_is_function (func_obj_val));

    return qking_return (ecma_op_function_call (ecma_get_object_from_value (func_obj_val),
                                                this_val,
                                                args_p,
                                                args_count));
  }
} /* qking_invoke_function */

/**
 * Call function specified by a function value
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned qking value of the called function
 */
qking_value_t
qking_call_function (const qking_value_t func_obj_val, /**< function object to call */
                     const qking_value_t this_val, /**< object for 'this' binding */
                     const qking_value_t args_p[], /**< function's call arguments */
                     qking_size_t args_count) /**< number of the arguments */
{
  qking_assert_api_available ();

  if (qking_value_is_function (func_obj_val))
  {
    return qking_invoke_function (false, func_obj_val, this_val, args_p, args_count);
  }

  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
} /* qking_call_function */

/**
 * Construct object value invoking specified function value as a constructor
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned qking value of the invoked constructor
 */
qking_value_t
qking_construct_object (const qking_value_t func_obj_val, /**< function object to call */
                        const qking_value_t args_p[], /**< function's call arguments
                                                       *   (NULL if arguments number is zero) */
                        qking_size_t args_count) /**< number of the arguments */
{
  qking_assert_api_available ();

  if (qking_value_is_constructor (func_obj_val))
  {
    ecma_value_t this_val = ECMA_VALUE_UNDEFINED;
    return qking_invoke_function (true, func_obj_val, this_val, args_p, args_count);
  }

  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
} /* qking_construct_object */

/**
 * Get keys of the specified object value
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return array object value - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_get_object_keys (const qking_value_t obj_val) /**< object value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  return ecma_builtin_helper_object_get_properties (ecma_get_object_from_value (obj_val), true);
} /* qking_get_object_keys */

/**
 * Get the prototype of the specified object
 *
 * @return prototype object or null value - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_get_prototype (const qking_value_t obj_val) /**< object value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_object_t *proto_obj_p = ecma_get_object_prototype (ecma_get_object_from_value (obj_val));

  if (proto_obj_p == NULL)
  {
    return ECMA_VALUE_NULL;
  }

  return ecma_make_object_value (proto_obj_p);
} /* qking_get_prototype */

/**
 * Set the prototype of the specified object
 *
 * @return true value - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_set_prototype (const qking_value_t obj_val, /**< object value */
                     const qking_value_t proto_obj_val) /**< prototype object value */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || ecma_is_value_error_reference (proto_obj_val)
      || (!ecma_is_value_object (proto_obj_val) && !ecma_is_value_null (proto_obj_val)))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  if (ecma_is_value_null (proto_obj_val))
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_val)->prototype_or_outer_reference_cp, NULL);
  }
  else
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_val)->prototype_or_outer_reference_cp,
                      ecma_get_object_from_value (proto_obj_val));
  }

  return ECMA_VALUE_TRUE;
} /* qking_set_prototype */

/**
 * Traverse objects.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
qking_objects_foreach (qking_objects_foreach_t foreach_p, /**< function pointer of the iterator function */
                       void *user_data_p) /**< pointer to user data */
{
  qking_assert_api_available ();

  QKING_ASSERT (foreach_p != NULL);

  for (ecma_object_t *iter_p = QKING_CONTEXT (ecma_gc_objects_p);
       iter_p != NULL;
       iter_p = ECMA_GET_POINTER (ecma_object_t, iter_p->gc_next_cp))
  {
    if (!ecma_is_lexical_environment (iter_p)
        && !foreach_p (ecma_make_object_value (iter_p), user_data_p))
    {
      return true;
    }
  }

  return false;
} /* qking_objects_foreach */

/**
 * Traverse objects having a given native type info.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
qking_objects_foreach_by_native_info (const qking_object_native_info_t *native_info_p, /**< the type info
                                                                                        *   of the native pointer */
                                      qking_objects_foreach_by_native_info_t foreach_p, /**< function to apply for
                                                                                         *   each matching object */
                                      void *user_data_p) /**< pointer to user data */
{
  qking_assert_api_available ();

  QKING_ASSERT (native_info_p != NULL);
  QKING_ASSERT (foreach_p != NULL);

  ecma_native_pointer_t *native_pointer_p;

  for (ecma_object_t *iter_p = QKING_CONTEXT (ecma_gc_objects_p);
       iter_p != NULL;
       iter_p = ECMA_GET_POINTER (ecma_object_t, iter_p->gc_next_cp))
  {
    if (!ecma_is_lexical_environment (iter_p))
    {
      native_pointer_p = ecma_get_native_pointer_value (iter_p);
      if (native_pointer_p
          && ((const qking_object_native_info_t *) native_pointer_p->info_p) == native_info_p
          && !foreach_p (ecma_make_object_value (iter_p), native_pointer_p->data_p, user_data_p))
      {
        return true;
      }
    }
  }

  return false;
} /* qking_objects_foreach_by_native_info */

/**
 * Get native pointer and its type information, associated with specified object.
 *
 * Note:
 *  If native pointer is present, its type information is returned
 *  in out_native_pointer_p and out_native_info_p.
 *
 * @return true - if there is an associated pointer,
 *         false - otherwise
 */
bool
qking_get_object_native_pointer (const qking_value_t obj_val, /**< object to get native pointer from */
                                 void **out_native_pointer_p, /**< [out] native pointer */
                                 const qking_object_native_info_t **out_native_info_p) /**< [out] the type info
                                                                                        *   of the native pointer */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return false;
  }

  ecma_native_pointer_t *native_pointer_p;
  native_pointer_p = ecma_get_native_pointer_value (ecma_get_object_from_value (obj_val));

  if (native_pointer_p == NULL)
  {
    return false;
  }

  if (out_native_pointer_p != NULL)
  {
    *out_native_pointer_p = native_pointer_p->data_p;
  }

  if (out_native_info_p != NULL)
  {
    *out_native_info_p = (const qking_object_native_info_t *) native_pointer_p->info_p;
  }

  return true;
} /* qking_get_object_native_pointer */

/**
 * Set native pointer and an optional type info for the specified object.
 *
 *
 * Note:
 *      If native pointer was already set for the object, its value is updated.
 *
 * Note:
 *      If a non-NULL free callback is specified in the native type info,
 *      it will be called by the garbage collector when the object is freed.
 *      The type info is always overwrites the previous value, so passing
 *      a NULL value deletes the current type info.
 */
void
qking_set_object_native_pointer (const qking_value_t obj_val, /**< object to set native pointer in */
                                 void *native_pointer_p, /**< native pointer */
                                 const qking_object_native_info_t *native_info_p) /**< object's native type info */
{
  qking_assert_api_available ();

  if (ecma_is_value_object (obj_val))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (obj_val);

    ecma_create_native_pointer_property (object_p, native_pointer_p, (void *) native_info_p);
  }
} /* qking_set_object_native_pointer */

/**
 * Applies the given function to the every property in the object.
 *
 * @return true - if object fields traversal was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in object fields traversal;
 *                - object fields traversal was stopped on callback that returned false;
 *         false - otherwise,
 *                 if getter of field threw a exception or unhandled exceptions were thrown during traversal;
 */
bool
qking_foreach_object_property (const qking_value_t obj_val, /**< object value */
                               qking_object_property_foreach_t foreach_p, /**< foreach function */
                               void *user_data_p) /**< user data for foreach function */
{
  return qking_foreach_object_property_of(obj_val,foreach_p,user_data_p,false, true, true);
} /* qking_foreach_object_property */

/**
 * Applies the given function to the every property in the object.
 *
 * @return true - if object fields traversal was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in object fields traversal;
 *                - object fields traversal was stopped on callback that returned false;
 *         false - otherwise,
 *                 if getter of field threw a exception or unhandled exceptions were thrown during traversal;
 */
bool
qking_foreach_object_property_of (const qking_value_t obj_val,
                                  qking_object_property_foreach_t foreach_p,
                                  void *user_data_p,
                                  bool is_array_indices_only, /**< true - exclude properties with names
                                                                *          that are not indices */
                                  bool is_enumerable_only, /**< true - exclude non-enumerable properties */
                                  bool is_with_prototype_chain) /**< true - list properties from prototype chain,
                                                                  *   false - list only own properties */
{
  qking_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return false;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (obj_val);
  ecma_collection_header_t *names_p = ecma_op_object_get_property_names (object_p, is_array_indices_only, is_enumerable_only, is_with_prototype_chain);
  ecma_value_t *ecma_value_p = ecma_collection_iterator_init (names_p);

  ecma_value_t property_value = ECMA_VALUE_EMPTY;

  bool continuous = true;

  while (continuous && ecma_value_p != NULL)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*ecma_value_p);

    property_value = ecma_op_object_get (object_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (property_value))
    {
      break;
    }

    continuous = foreach_p (*ecma_value_p, property_value, user_data_p);
    ecma_free_value (property_value);

    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }

  ecma_free_values_collection (names_p, 0);

  if (!ECMA_IS_VALUE_ERROR (property_value))
  {
    return true;
  }

  ecma_free_value (QKING_CONTEXT (error_value));
  return false;
}

/**
 * Resolve or reject the promise with an argument.
 *
 * @return undefined value - if success
 *         value marked with error flag - otherwise
 */
qking_value_t
qking_resolve_or_reject_promise (qking_value_t promise, /**< the promise value */
                                 qking_value_t argument, /**< the argument */
                                 bool is_resolve) /**< whether the promise should be resolved or rejected */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  if (!ecma_is_value_object (promise) || !ecma_is_promise (ecma_get_object_from_value (promise)))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  lit_magic_string_id_t prop_name = (is_resolve ? LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION
                                                : LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION);

  ecma_value_t function = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (promise), prop_name);

  ecma_value_t ret = ecma_op_function_call (ecma_get_object_from_value (function),
                                            ECMA_VALUE_UNDEFINED,
                                            &argument,
                                            1);

  ecma_free_value (function);

  return ret;
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  QKING_UNUSED (promise);
  QKING_UNUSED (argument);
  QKING_UNUSED (is_resolve);

  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Promise not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* qking_resolve_or_reject_promise */

/**
 * Validate UTF-8 string
 *
 * @return true - if UTF-8 string is well-formed
 *         false - otherwise
 */
bool
qking_is_valid_utf8_string (const qking_char_t *utf8_buf_p, /**< UTF-8 string */
                            qking_size_t buf_size) /**< string size */
{
  return lit_is_valid_utf8_string ((lit_utf8_byte_t *) utf8_buf_p,
                                   (lit_utf8_size_t) buf_size);
} /* qking_is_valid_utf8_string */

/**
 * Validate CESU-8 string
 *
 * @return true - if CESU-8 string is well-formed
 *         false - otherwise
 */
bool
qking_is_valid_cesu8_string (const qking_char_t *cesu8_buf_p, /**< CESU-8 string */
                             qking_size_t buf_size) /**< string size */
{
  return lit_is_valid_cesu8_string ((lit_utf8_byte_t *) cesu8_buf_p,
                                    (lit_utf8_size_t) buf_size);
} /* qking_is_valid_cesu8_string */

/**
 * Allocate memory on the engine's heap.
 *
 * Note:
 *      This function may take away memory from the executed JavaScript code.
 *      If any other dynamic memory allocation API is available (e.g., libc
 *      malloc), it should be used instead.
 *
 * @return allocated memory on success
 *         NULL otherwise
 */
void *
qking_heap_alloc (size_t size) /**< size of the memory block */
{
  qking_assert_api_available ();

  return jmem_heap_alloc_block_null_on_error (size);
} /* qking_heap_alloc */

/**
 * Free memory allocated on the engine's heap.
 */
void
qking_heap_free (void *mem_p, /**< value returned by qking_heap_alloc */
                 size_t size) /**< same size as passed to qking_heap_alloc */
{
  qking_assert_api_available ();

  jmem_heap_free_block (mem_p, size);
} /* qking_heap_free */

/**
 * Create an external engine context.
 *
 * @return the pointer to the context.
 */
qking_context_t *
qking_create_context (uint32_t heap_size, /**< the size of heap */
                      qking_context_alloc_t alloc, /**< the alloc function */
                      void *cb_data_p) /**< the cb_data for alloc function */
{
  QKING_UNUSED (heap_size);

#ifdef QKING_ENABLE_EXTERNAL_CONTEXT

  size_t total_size = sizeof (qking_context_t) + JMEM_ALIGNMENT;

#ifndef QKING_SYSTEM_ALLOCATOR
  heap_size = QKING_ALIGNUP (heap_size, JMEM_ALIGNMENT);

  /* Minimum heap size is 1Kbyte. */
  if (heap_size < 1024)
  {
    return NULL;
  }

  total_size += heap_size;
#endif /* !QKING_SYSTEM_ALLOCATOR */

  total_size = QKING_ALIGNUP (total_size, JMEM_ALIGNMENT);

  qking_context_t *context_p = (qking_context_t *) alloc (total_size, cb_data_p);

  if (context_p == NULL)
  {
    return NULL;
  }

  memset (context_p, 0, total_size);

  uintptr_t context_ptr = ((uintptr_t) context_p) + sizeof (qking_context_t);
  context_ptr = QKING_ALIGNUP (context_ptr, (uintptr_t) JMEM_ALIGNMENT);

  uint8_t *byte_p = (uint8_t *) context_ptr;

#ifndef QKING_SYSTEM_ALLOCATOR
  context_p->heap_p = (jmem_heap_t *) byte_p;
  context_p->heap_size = heap_size;
  byte_p += heap_size;
#endif /* !QKING_SYSTEM_ALLOCATOR */

  QKING_ASSERT (byte_p <= ((uint8_t *) context_p) + total_size);

  QKING_UNUSED (byte_p);
  return context_p;

#else /* !QKING_ENABLE_EXTERNAL_CONTEXT */

  QKING_UNUSED (alloc);
  QKING_UNUSED (cb_data_p);

  return NULL;

#endif /* QKING_ENABLE_EXTERNAL_CONTEXT */
} /* qking_create_context */

/**
 * If QKING_VM_EXEC_STOP is defined the callback passed to this function is
 * periodically called with the user_p argument. If frequency is greater
 * than 1, the callback is only called at every frequency ticks.
 */
void
qking_set_vm_exec_stop_callback (qking_vm_exec_stop_callback_t stop_cb, /**< periodically called user function */
                                 void *user_p, /**< pointer passed to the function */
                                 uint32_t frequency) /**< frequency of the function call */
{
#ifdef QKING_VM_EXEC_STOP
  if (frequency == 0)
  {
    frequency = 1;
  }

  QKING_CONTEXT (vm_exec_stop_frequency) = frequency;
  QKING_CONTEXT (vm_exec_stop_counter) = frequency;
  QKING_CONTEXT (vm_exec_stop_user_p) = user_p;
  QKING_CONTEXT (vm_exec_stop_cb) = stop_cb;
#else /* !QKING_VM_EXEC_STOP */
  QKING_UNUSED (stop_cb);
  QKING_UNUSED (user_p);
  QKING_UNUSED (frequency);
#endif /* QKING_VM_EXEC_STOP */
} /* qking_set_vm_exec_stop_callback */

/**
 * Check if the given value is an ArrayBuffer object.
 *
 * @return true - if it is an ArrayBuffer object
 *         false - otherwise
 */
bool
qking_value_is_arraybuffer (const qking_value_t value) /**< value to check if it is an ArrayBuffer */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  return ecma_is_arraybuffer (value);
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
  return false;
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_value_is_arraybuffer */

/**
 * Creates an ArrayBuffer object with the given length (size).
 *
 * Notes:
 *      * the length is specified in bytes.
 *      * returned value must be freed with qking_release_value, when it is no longer needed.
 *      * if the typed arrays are disabled this will return a TypeError.
 *
 * @return value of the constructed ArrayBuffer object
 */
qking_value_t
qking_create_arraybuffer (const qking_length_t size) /**< size of the ArrayBuffer to create */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  return qking_return (ecma_make_object_value (ecma_arraybuffer_new_object (size)));
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (size);
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_create_arraybuffer */

/**
 * Creates an ArrayBuffer object with user specified buffer.
 *
 * Notes:
 *     * the size is specified in bytes.
 *     * the buffer passed should be at least the specified bytes big.
 *     * if the typed arrays are disabled this will return a TypeError.
 *     * if the size is zero or the buffer_p is a null pointer this will return a RangeError.
 *
 * @return value of the construced ArrayBuffer object
 */
qking_value_t
qking_create_arraybuffer_external (const qking_length_t size, /**< size of the buffer to used */
                                   uint8_t *buffer_p, /**< buffer to use as the ArrayBuffer's backing */
                                   qking_object_native_free_callback_t free_cb) /**< buffer free callback */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (size == 0 || buffer_p == NULL)
  {
    return qking_throw (ecma_raise_range_error (ECMA_ERR_MSG ("invalid buffer size or storage reference")));
  }

  ecma_object_t *arraybuffer = ecma_arraybuffer_new_object_external (size,
                                                                     buffer_p,
                                                                     (ecma_object_native_free_callback_t) free_cb);
  return qking_return (ecma_make_object_value (arraybuffer));
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (size);
  QKING_UNUSED (buffer_p);
  QKING_UNUSED (free_cb);
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_create_arraybuffer_external */

/**
 * Copy bytes into the ArrayBuffer from a buffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer will return 0.
 *
 * @return number of bytes copied into the ArrayBuffer.
 */
qking_length_t
qking_arraybuffer_write (const qking_value_t value, /**< target ArrayBuffer */
                         qking_length_t offset, /**< start offset of the ArrayBuffer */
                         const uint8_t *buf_p, /**< buffer to copy from */
                         qking_length_t buf_size) /**< number of bytes to copy from the buffer */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (!ecma_is_arraybuffer (value))
  {
    return 0;
  }

  ecma_object_t *buffer_p = ecma_get_object_from_value (value);
  qking_length_t length = ecma_arraybuffer_get_length (buffer_p);

  if (offset >= length)
  {
    return 0;
  }

  qking_length_t copy_count = QKING_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *mem_buffer_p = ecma_arraybuffer_get_buffer (buffer_p);

    memcpy ((void *) (mem_buffer_p + offset), (void *) buf_p, copy_count);
  }

  return copy_count;
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
  QKING_UNUSED (offset);
  QKING_UNUSED (buf_p);
  QKING_UNUSED (buf_size);
  return 0;
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_arraybuffer_write */

/**
 * Copy bytes from a buffer into an ArrayBuffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer will return 0.
 *
 * @return number of bytes read from the ArrayBuffer.
 */
qking_length_t
qking_arraybuffer_read (const qking_value_t value, /**< ArrayBuffer to read from */
                        qking_length_t offset, /**< start offset of the ArrayBuffer */
                        uint8_t *buf_p, /**< destination buffer to copy to */
                        qking_length_t buf_size) /**< number of bytes to copy into the buffer */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (!ecma_is_arraybuffer (value))
  {
    return 0;
  }

  ecma_object_t *buffer_p = ecma_get_object_from_value (value);
  qking_length_t length = ecma_arraybuffer_get_length (buffer_p);

  if (offset >= length)
  {
    return 0;
  }

  qking_length_t copy_count = QKING_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *mem_buffer_p = ecma_arraybuffer_get_buffer (buffer_p);

    memcpy ((void *) buf_p, (void *) (mem_buffer_p + offset), copy_count);
  }

  return copy_count;
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
  QKING_UNUSED (offset);
  QKING_UNUSED (buf_p);
  QKING_UNUSED (buf_size);
  return 0;
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_arraybuffer_read */

/**
 * Get the length (size) of the ArrayBuffer in bytes.
 *
 * Note:
 *     This is the 'byteLength' property of an ArrayBuffer.
 *
 * @return the length of the ArrayBuffer in bytes.
 */
qking_length_t
qking_get_arraybuffer_byte_length (const qking_value_t value) /**< ArrayBuffer */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (ecma_is_arraybuffer (value))
  {
    ecma_object_t *buffer_p = ecma_get_object_from_value (value);
    return ecma_arraybuffer_get_length (buffer_p);
  }
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  return 0;
} /* qking_get_arraybuffer_byte_length */

/**
 * Get a pointer for the start of the ArrayBuffer.
 *
 * Note:
 *    * Only valid for ArrayBuffers created with qking_create_arraybuffer_external.
 *    * This is a high-risk operation as the bounds are not checked
 *      when accessing the pointer elements.
 *    * qking_release_value must be called on the ArrayBuffer when the pointer is no longer needed.
 *
 * @return pointer to the back-buffer of the ArrayBuffer.
 *         pointer is NULL if the parameter is not an ArrayBuffer with external memory
             or it is not an ArrayBuffer at all.
 */
uint8_t *
qking_get_arraybuffer_pointer (const qking_value_t value) /**< Array Buffer to use */
{
  qking_assert_api_available ();
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (!ecma_is_arraybuffer (value))
  {
    return NULL;
  }

  ecma_object_t *buffer_p = ecma_get_object_from_value (value);
  if (ECMA_ARRAYBUFFER_HAS_EXTERNAL_MEMORY (buffer_p))
  {
    qking_acquire_value (value);
    lit_utf8_byte_t *mem_buffer_p = ecma_arraybuffer_get_buffer (buffer_p);
    return (uint8_t *const) mem_buffer_p;
  }
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

  return NULL;
} /* qking_get_arraybuffer_pointer */


/**
 * TypedArray related functions
 */

/**
 * Check if the given value is a TypedArray object.
 *
 * @return true - if it is a TypedArray object
 *         false - otherwise
 */
bool
qking_value_is_typedarray (qking_value_t value) /**< value to check if it is a TypedArray */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  return ecma_is_typedarray (value);
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
  return false;
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_value_is_typedarray */

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
/**
 * TypedArray mapping type
 */
typedef struct
{
  qking_typedarray_type_t api_type; /**< api type */
  ecma_builtin_id_t prototype_id; /**< prototype ID */
  lit_magic_string_id_t lit_id; /**< literal ID */
  uint8_t element_size_shift; /**< element size shift */
} qking_typedarray_mapping_t;

/**
 * List of TypedArray mappings
 */
static qking_typedarray_mapping_t qking_typedarray_mappings[] =
{
#define TYPEDARRAY_ENTRY(NAME, LIT_NAME, SIZE_SHIFT) \
  { QKING_TYPEDARRAY_ ## NAME, ECMA_BUILTIN_ID_ ## NAME ## ARRAY_PROTOTYPE, \
    LIT_MAGIC_STRING_ ## LIT_NAME ## _ARRAY_UL, SIZE_SHIFT }

  TYPEDARRAY_ENTRY (UINT8, UINT8, 0),
  TYPEDARRAY_ENTRY (UINT8CLAMPED, UINT8_CLAMPED, 0),
  TYPEDARRAY_ENTRY (INT8, INT8, 0),
  TYPEDARRAY_ENTRY (UINT16, UINT16, 1),
  TYPEDARRAY_ENTRY (INT16, INT16, 1),
  TYPEDARRAY_ENTRY (UINT32, UINT32, 2),
  TYPEDARRAY_ENTRY (INT32, INT32, 2),
  TYPEDARRAY_ENTRY (FLOAT32, FLOAT32, 2),
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
  TYPEDARRAY_ENTRY (FLOAT64, FLOAT64, 3),
#endif

#undef TYPEDARRAY_ENTRY
};

/**
 * Helper function to get the TypedArray prototype, literal id, and element size shift
 * information.
 *
 * @return true - if the TypedArray information was found
 *         false - if there is no such TypedArray type
 */
static bool
qking_typedarray_find_by_type (qking_typedarray_type_t type_name, /**< type of the TypedArray */
                               ecma_builtin_id_t *prototype_id, /**< [out] found prototype object id */
                               lit_magic_string_id_t *lit_id, /**< [out] found literal id */
                               uint8_t *element_size_shift) /**< [out] found element size shift value */
{
  QKING_ASSERT (prototype_id != NULL);
  QKING_ASSERT (lit_id != NULL);
  QKING_ASSERT (element_size_shift != NULL);

  for (uint32_t i = 0; i < sizeof (qking_typedarray_mappings) / sizeof (qking_typedarray_mappings[0]); i++)
  {
    if (type_name == qking_typedarray_mappings[i].api_type)
    {
      *prototype_id = qking_typedarray_mappings[i].prototype_id;
      *lit_id = qking_typedarray_mappings[i].lit_id;
      *element_size_shift = qking_typedarray_mappings[i].element_size_shift;
      return true;
    }
  }

  return false;
} /* qking_typedarray_find_by_type */

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

/**
 * Create a TypedArray object with a given type and length.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * byteOffset property will be set to 0.
 *      * byteLength property will be a multiple of the length parameter (based on the type).
 *
 * @return - new TypedArray object
 */
qking_value_t
qking_create_typedarray (qking_typedarray_type_t type_name, /**< type of TypedArray to create */
                         qking_length_t length) /**< element count of the new TypedArray */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  ecma_builtin_id_t prototype_id = 0;
  lit_magic_string_id_t lit_id = 0;
  uint8_t element_size_shift = 0;

  if (!qking_typedarray_find_by_type (type_name, &prototype_id, &lit_id, &element_size_shift))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("incorrect type for TypedArray.")));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);

  ecma_value_t array_value = ecma_typedarray_create_object_with_length (length,
                                                                        prototype_obj_p,
                                                                        element_size_shift,
                                                                        lit_id);
  ecma_deref_object (prototype_obj_p);

  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (array_value));

  return array_value;
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (type_name);
  QKING_UNUSED (length);
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_create_typedarray */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer, byteOffset, length)' equivalent call.
 *
 * @return - new TypedArray object
 */
qking_value_t
qking_create_typedarray_for_arraybuffer_sz (qking_typedarray_type_t type_name, /**< type of TypedArray to create */
                                            const qking_value_t arraybuffer, /**< ArrayBuffer to use */
                                            qking_length_t byte_offset, /**< offset for the ArrayBuffer */
                                            qking_length_t length) /**< number of elements to use from ArrayBuffer */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  ecma_builtin_id_t prototype_id = 0;
  lit_magic_string_id_t lit_id = 0;
  uint8_t element_size_shift = 0;

  if (!qking_typedarray_find_by_type (type_name, &prototype_id, &lit_id, &element_size_shift))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("incorrect type for TypedArray.")));
  }

  if (!ecma_is_arraybuffer (arraybuffer))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an ArrayBuffer")));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);
  ecma_value_t arguments_p[3] =
  {
    arraybuffer,
    ecma_make_uint32_value (byte_offset),
    ecma_make_uint32_value (length)
  };

  ecma_value_t array_value = ecma_op_create_typedarray (arguments_p, 3, prototype_obj_p, element_size_shift, lit_id);
  ecma_free_value (arguments_p[1]);
  ecma_free_value (arguments_p[2]);
  ecma_deref_object (prototype_obj_p);

  return qking_return (array_value);
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (type_name);
  QKING_UNUSED (arraybuffer);
  QKING_UNUSED (byte_offset);
  QKING_UNUSED (length);
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_create_typedarray_for_arraybuffer_sz */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer)' equivalent call.
 *
 * @return - new TypedArray object
 */
qking_value_t
qking_create_typedarray_for_arraybuffer (qking_typedarray_type_t type_name, /**< type of TypedArray to create */
                                         const qking_value_t arraybuffer) /**< ArrayBuffer to use */
{
  qking_assert_api_available ();
  qking_length_t byteLength = qking_get_arraybuffer_byte_length (arraybuffer);
  return qking_create_typedarray_for_arraybuffer_sz (type_name, arraybuffer, 0, byteLength);
} /* qking_create_typedarray_for_arraybuffer */

/**
 * Get the type of the TypedArray.
 *
 * @return - type of the TypedArray
 *         - QKING_TYPEDARRAY_INVALID if the argument is not a TypedArray
 */
qking_typedarray_type_t
qking_get_typedarray_type (qking_value_t value) /**< object to get the TypedArray type */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (!ecma_is_typedarray (value))
  {
    return QKING_TYPEDARRAY_INVALID;
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);

  lit_magic_string_id_t class_name_id = ecma_object_get_class_name (array_p);
  for (uint32_t i = 0; i < sizeof (qking_typedarray_mappings) / sizeof (qking_typedarray_mappings[0]); i++)
  {
    if (class_name_id == qking_typedarray_mappings[i].lit_id)
    {
      return qking_typedarray_mappings[i].api_type;
    }
  }
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

  return QKING_TYPEDARRAY_INVALID;
} /* qking_get_typedarray_type */

/**
 * Get the element count of the TypedArray.
 *
 * @return length of the TypedArray.
 */
qking_length_t
qking_get_typedarray_length (qking_value_t value) /**< TypedArray to query */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (ecma_is_typedarray (value))
  {
    ecma_object_t *array_p = ecma_get_object_from_value (value);
    return ecma_typedarray_get_length (array_p);
  }
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

  return 0;
} /* qking_get_typedarray_length */

/**
 * Get the underlying ArrayBuffer from a TypedArray.
 *
 * Additionally the byteLength and byteOffset properties are also returned
 * which were specified when the TypedArray was created.
 *
 * Note:
 *     the returned value must be freed with a qking_release_value call
 *
 * @return ArrayBuffer of a TypedArray
 *         TypeError if the object is not a TypedArray.
 */
qking_value_t
qking_get_typedarray_buffer (qking_value_t value, /**< TypedArray to get the arraybuffer from */
                             qking_length_t *byte_offset, /**< [out] byteOffset property */
                             qking_length_t *byte_length) /**< [out] byteLength property */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
  if (!ecma_is_typedarray (value))
  {
    return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Object is not a TypedArray.")));
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);
  uint8_t shift = ecma_typedarray_get_element_size_shift (array_p);

  if (byte_length != NULL)
  {
    *byte_length = (qking_length_t) (ecma_typedarray_get_length (array_p) << shift);
  }

  if (byte_offset != NULL)
  {
    *byte_offset = (qking_length_t) ecma_typedarray_get_offset (array_p);
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (array_p);
  ecma_ref_object (arraybuffer_p);
  return qking_return (ecma_make_object_value (arraybuffer_p));
#else /* CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
  QKING_UNUSED (value);
  QKING_UNUSED (byte_length);
  QKING_UNUSED (byte_offset);
  return qking_throw (ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray is not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
} /* qking_get_typedarray_buffer */

/**
 * Create an object from JSON
 *
 * Note:
 *      The returned value must be freed with qking_release_value
 * @return qking_value_t from json formated string or an error massage
 */
qking_value_t
qking_json_parse (const qking_char_t *string_p, /**< json string */
                  qking_size_t string_size) /**< json string size */
{
  qking_assert_api_available ();

#ifndef CONFIG_DISABLE_JSON_BUILTIN
  ecma_value_t ret_value = ecma_builtin_json_parse_buffer (string_p, string_size);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = qking_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("JSON string parse error.")));
  }

  return ret_value;
#else /* CONFIG_DISABLE_JSON_BUILTIN */
  QKING_UNUSED (string_p);
  QKING_UNUSED (string_size);

  return qking_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("The JSON has been disabled.")));
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */
} /* qking_json_parse */

/**
 * Create a Json formated string from an object
 *
 * Note:
 *      The returned value must be freed with qking_release_value
 * @return json formated qking_value_t or an error massage
 */
qking_value_t
qking_json_stringify(const qking_value_t object_to_stringify) /**< a qking_object_t to stringify */
{
  qking_assert_api_available ();
#ifndef CONFIG_DISABLE_JSON_BUILTIN
  ecma_value_t ret_value = ecma_builtin_json_string_from_object (object_to_stringify);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = qking_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("JSON stringify error.")));
  }

  return ret_value;
#else /* CONFIG_DISABLE_JSON_BUILTIN */
  QKING_UNUSED (object_to_stringify);

  return qking_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("The JSON has been disabled.")));
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */
} /* qking_json_stringify */

qking_value_t qking_print_log(const char * prefix,
                              const qking_value_t func_obj_val,
                              const qking_value_t this_p,
                              const qking_value_t *args_p,
                              const qking_length_t args_cnt) {
  (void)func_obj_val; /* unused */
  (void)this_p;       /* unused */

  qking_value_t ret_val = qking_create_undefined();
  size_t all_size = 1;

  const char ** str_array = (const char**)jmem_system_malloc(sizeof(const char *)*args_cnt);

  for (qking_length_t arg_index = 0; arg_index < args_cnt; arg_index++) {
    qking_value_t str_val = qking_value_to_string(args_p[arg_index]);

    if (qking_value_is_error(str_val)) {
      /* There is no need to free the undefined value. */
      ret_val = str_val;
      for (int i = 0; i < arg_index; ++i) {
        jmem_system_free((void*)str_array[i]);
      }
      jmem_system_free(str_array);
      return ret_val;
    }

    const char *a_string = qking_convert_to_log_str_from_value(str_val);
    all_size += strlen(a_string) + 1;
    str_array[arg_index] = a_string;

    qking_release_value(str_val);
  }

  char * final_str = (char *)jmem_system_malloc(sizeof(char) * all_size);
  char * copy_start = final_str;
  for (int i = 0; i < args_cnt; ++i) {
    size_t len = strlen(str_array[i]);
    strcpy(copy_start, str_array[i]);
    copy_start[len] = ' ';
    copy_start += (len + 1);
  }
  final_str[all_size - 1] = '\0';

  LOGI("%s %s",prefix, final_str);

  jmem_system_free(final_str);
  for (int i = 0; i < args_cnt; ++i) {
    jmem_system_free((void *)str_array[i]);
  }
  jmem_system_free(str_array);

  return ret_val;
} /* qking_print_log */

/**
 * @}
 */
