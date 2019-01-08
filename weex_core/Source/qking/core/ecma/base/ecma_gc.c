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
/**
 * Garbage collector implementation
 */

#include "ecma_alloc.h"
#include "ecma_globals.h"
#include "ecma_gc.h"
#include "ecma_helpers.h"
#include "ecma_property_hashmap.h"
#include "jcontext.h"
#include "jrt.h"
#include "jrt_libc_includes.h"
#include "jrt_bit_fields.h"
#include "re_compiler.h"
#include "vm_defines.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
#include "ecma_typedarray_object.h"
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
#include "ecma_promise_object.h"
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN
#include "ecma_map_object.h"
#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */
#include "ecma_function_object.h"

/* TODO: Extract GC to a separate component */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

/*
 * The garbage collector uses the reference counter
 * of object: it increases the counter by one when
 * the object is marked at the first time.
 */

#ifdef QKING_ENABLE_GC_DEBUG

static qking_value_t s_gc_mark = 0;
static qking_value_t s_gc_alive[10240] = { 0 };
static uint32_t s_gc_alive_times[10240] = { 0 };
static uint32_t s_gc_alive_count = 0;

static qking_value_t s_gc_ignore[2048] = { 0 };
static uint32_t s_gc_ignore_count = 0;
static bool s_gc_start_ignore = false;

static int ecma_gc_check_alive(ecma_value_t alive) {
    int index = -1;
    for (int i = 0; i < s_gc_alive_count; i++) {
        if (s_gc_alive[i] == alive) {
            index = i;
            s_gc_alive_times[i]++;
            if (s_gc_alive_times[i] > 30) {
                ecma_object_t *object_p = ecma_get_object_from_value(alive);
                uint16_t ref = object_p->type_flags_refs >> 6;
                if (ref > 0) {
                    printf("[gc][leak][%i]=>[ref][%d][time][%d]\n", ecma_make_object_value(object_p), ref + 1, s_gc_alive_times[i]);
                }
            }
            break;
        }
    }
    return index;
}

static void ecma_gc_free_alive(ecma_value_t alive) {
    for (int i = 0; i < s_gc_alive_count; i++) {
        if (s_gc_alive[i] == alive) {
            if (s_gc_alive_count - i - 1 > 0) {
                memcpy(s_gc_alive + i, s_gc_alive + i + 1, sizeof(ecma_value_t) * (s_gc_alive_count - i - 1));
                memcpy(s_gc_alive_times + i, s_gc_alive_times + i + 1, sizeof(uint32_t) * (s_gc_alive_count - i - 1));
            }
            s_gc_alive_count--;
            break;
        }
    }
}

static void ecma_gc_info_alive(ecma_value_t alive) {
    int index = ecma_gc_check_alive(alive);
    if (index < 0) {
        s_gc_alive[s_gc_alive_count] = alive;
        s_gc_alive_times[s_gc_alive_count]++;
        s_gc_alive_count++;
    }
}

#ifdef JMEM_STATS
static size_t s_gc_allocated = 0;
#endif
static uint32_t s_gc_objects = 0;
static uint32_t s_gc_free_objects = 0;

void ecma_gc_info_ignore(ecma_value_t ignore) {
    s_gc_ignore[s_gc_ignore_count++] = ignore;
}

#endif

/**
 * Get next object in list of objects with same generation.
 *
 * @return pointer to the next ecma-object
 *         NULL - if there is no next ecma-object
 */
static inline ecma_object_t *
ecma_gc_get_object_next (ecma_object_t *object_p) /**< object */
{
  QKING_ASSERT (object_p != NULL);

  return ECMA_GET_POINTER (ecma_object_t, object_p->gc_next_cp);
} /* ecma_gc_get_object_next */

/**
 * Set next object in list of objects with same generation.
 */
static inline void
ecma_gc_set_object_next (ecma_object_t *object_p, /**< object */
                         ecma_object_t *next_object_p) /**< next object */
{
  QKING_ASSERT (object_p != NULL);

  ECMA_SET_POINTER (object_p->gc_next_cp, next_object_p);
} /* ecma_gc_set_object_next */

/**
 * Get visited flag of the object.
 *
 * @return true  - if visited
 *         false - otherwise
 */
static inline bool
ecma_gc_is_object_visited (ecma_object_t *object_p) /**< object */
{
  QKING_ASSERT (object_p != NULL);

  return (object_p->type_flags_refs >= ECMA_OBJECT_REF_ONE);
} /* ecma_gc_is_object_visited */

/**
 * Set visited flag of the object.
 */
static inline void
ecma_gc_set_object_visited (ecma_object_t *object_p) /**< object */
{
#ifdef QKING_ENABLE_GC_DEBUG
    QKING_ASSERT(s_gc_mark > 0);
    printf("[gc][%d]=>[visited][%d]\n", ecma_make_object_value(object_p), s_gc_mark);
#endif
  /* Set reference counter to one if it is zero. */
  if (object_p->type_flags_refs < ECMA_OBJECT_REF_ONE)
  {
    object_p->type_flags_refs |= ECMA_OBJECT_REF_ONE;
  }
} /* ecma_gc_set_object_visited */

/**
 * Initialize GC information for the object
 */
inline void
ecma_init_gc_info (ecma_object_t *object_p) /**< object */
{
#ifdef QKING_ENABLE_GC_DEBUG
  if (s_gc_start_ignore) {
      ecma_gc_info_ignore(ecma_make_object_value(object_p));
  }
  printf("[gc][%i]=>[new]\n", ecma_make_object_value(object_p));
#endif
  QKING_CONTEXT (ecma_gc_objects_number)++;
  QKING_CONTEXT (ecma_gc_new_objects)++;

  QKING_ASSERT (QKING_CONTEXT (ecma_gc_new_objects) <= QKING_CONTEXT (ecma_gc_objects_number));

  QKING_ASSERT (object_p->type_flags_refs < ECMA_OBJECT_REF_ONE);
  object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs | ECMA_OBJECT_REF_ONE);

  ecma_gc_set_object_next (object_p, QKING_CONTEXT (ecma_gc_objects_p));
  QKING_CONTEXT (ecma_gc_objects_p) = object_p;
} /* ecma_init_gc_info */

/**
 * Increase reference counter of an object
 */
void
ecma_ref_object (ecma_object_t *object_p) /**< object */
{
#ifdef QKING_ENABLE_GC_DEBUG
    uint16_t ref = object_p->type_flags_refs >> 6;
    printf("[gc][%i]=>[ref][%d]\n", ecma_make_object_value(object_p), ref + 1);
#endif
  if (QKING_LIKELY (object_p->type_flags_refs < ECMA_OBJECT_MAX_REF))
  {
    object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs + ECMA_OBJECT_REF_ONE);
  }
  else
  {
    qking_fatal (ERR_REF_COUNT_LIMIT);
  }
} /* ecma_ref_object */

/**
 * Decrease reference counter of an object
 */
inline void QKING_ATTR_ALWAYS_INLINE
ecma_deref_object (ecma_object_t *object_p) /**< object */
{
#ifdef QKING_ENABLE_GC_DEBUG
    uint16_t ref = object_p->type_flags_refs >> 6;
    printf("[gc][%i]=>[deref][%d]\n", ecma_make_object_value(object_p), ref - 1);
#endif
  QKING_ASSERT (object_p->type_flags_refs >= ECMA_OBJECT_REF_ONE);
  object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs - ECMA_OBJECT_REF_ONE);
} /* ecma_deref_object */

/**
 * Mark referenced object from closure
 */
static void ecma_gc_mark_closure (ecma_object_t *func_obj_p) {
    ecma_closure_t **closure_list_pp = NULL;
    const ecma_compiled_code_t *bytecode_data_p = NULL;
    if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_ARROW_FUNCTION)
    {
        ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_obj_p;
        closure_list_pp = JMEM_CP_GET_POINTER(ecma_closure_t *, arrow_func_p->closure_cp);
        bytecode_data_p = ecma_op_arrow_function_get_compiled_code (arrow_func_p);
    }
    else {
        ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;
        closure_list_pp = JMEM_CP_GET_POINTER(ecma_closure_t *, ext_func_p->u.function.closure_cp);
        bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);
    }
    uint32_t in_closure = ((ecma_compiled_function_state_t *)bytecode_data_p)->in_closure;
    QKING_ASSERT(in_closure > 0);
    for (int i = 0; i < in_closure; i++) {
        ecma_closure_t *closure_p = closure_list_pp[i];
        ecma_value_t var = closure_p->var;
        if (ecma_is_value_object(var)) {
            ecma_object_t *value_obj_p = ecma_get_object_from_value (var);
            ecma_gc_set_object_visited (value_obj_p);
        }
    }
}

/**
 * Mark referenced object from property
 */
static void
ecma_gc_mark_property (ecma_property_pair_t *property_pair_p, /**< property pair */
                       uint32_t index) /**< property index */
{
  uint8_t property = property_pair_p->header.types[index];

  switch (ECMA_PROPERTY_GET_TYPE (property))
  {
    case ECMA_PROPERTY_TYPE_NAMEDDATA:
    {
      ecma_value_t value = property_pair_p->values[index].value;

      if (ecma_is_value_object (value))
      {
        ecma_object_t *value_obj_p = ecma_get_object_from_value (value);

        ecma_gc_set_object_visited (value_obj_p);
      }
      break;
    }
    case ECMA_PROPERTY_TYPE_NAMEDACCESSOR:
    {
      ecma_property_value_t *accessor_objs_p = property_pair_p->values + index;
      ecma_object_t *getter_obj_p = ecma_get_named_accessor_property_getter (accessor_objs_p);
      ecma_object_t *setter_obj_p = ecma_get_named_accessor_property_setter (accessor_objs_p);

      if (getter_obj_p != NULL)
      {
        ecma_gc_set_object_visited (getter_obj_p);
      }

      if (setter_obj_p != NULL)
      {
        ecma_gc_set_object_visited (setter_obj_p);
      }
      break;
    }
    case ECMA_PROPERTY_TYPE_INTERNAL:
    {
      QKING_ASSERT (ECMA_PROPERTY_GET_NAME_TYPE (property) == ECMA_DIRECT_STRING_MAGIC
                    && property_pair_p->names_cp[index] >= LIT_FIRST_INTERNAL_MAGIC_STRING);
      break;
    }
    default:
    {
      QKING_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_SPECIAL);

      QKING_ASSERT (property == ECMA_PROPERTY_TYPE_HASHMAP
                    || property == ECMA_PROPERTY_TYPE_DELETED);
      break;
    }
  }
} /* ecma_gc_mark_property */

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

/**
 * Mark objects referenced by Promise built-in.
 */
static void
ecma_gc_mark_promise_object (ecma_extended_object_t *ext_object_p) /**< extended object */
{
  /* Mark promise result. */
  ecma_value_t result = ext_object_p->u.class_prop.u.value;

  if (ecma_is_value_object (result))
  {
    ecma_gc_set_object_visited (ecma_get_object_from_value (result));
  }

  /* Mark all reactions. */
  ecma_value_t *ecma_value_p;
  ecma_value_p = ecma_collection_iterator_init (((ecma_promise_object_t *) ext_object_p)->fulfill_reactions);

  while (ecma_value_p != NULL)
  {
    ecma_gc_set_object_visited (ecma_get_object_from_value (*ecma_value_p));
    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }

  ecma_value_p = ecma_collection_iterator_init (((ecma_promise_object_t *) ext_object_p)->reject_reactions);

  while (ecma_value_p != NULL)
  {
    ecma_gc_set_object_visited (ecma_get_object_from_value (*ecma_value_p));
    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }
} /* ecma_gc_mark_promise_object */

#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */

#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN

/**
 * Mark objects referenced by Map built-in.
 */
static void
ecma_gc_mark_map_object (ecma_extended_object_t *ext_object_p) /**< extended object */
{
  ecma_map_object_t *map_object_p = (ecma_map_object_t *) ext_object_p;

  jmem_cpointer_t first_chunk_cp = map_object_p->first_chunk_cp;

  if (QKING_UNLIKELY (first_chunk_cp == ECMA_NULL_POINTER))
  {
    return;
  }

  ecma_value_t *item_p = ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t, first_chunk_cp)->items;

  while (true)
  {
    ecma_value_t item = *item_p++;

    if (!ecma_is_value_pointer (item))
    {
      if (ecma_is_value_object (item))
      {
        ecma_gc_set_object_visited (ecma_get_object_from_value (item));
      }
    }
    else
    {
      item_p = (ecma_value_t *) ecma_get_pointer_from_value (item);

      if (item_p == NULL)
      {
        return;
      }
    }
  }
} /* ecma_gc_mark_map_object */

#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */

/**
 * Mark objects as visited starting from specified object as root
 */
static void
ecma_gc_mark (ecma_object_t *object_p) /**< object to mark from */
{
#ifdef QKING_ENABLE_GC_DEBUG
  s_gc_mark = ecma_make_object_value(object_p);
#endif
    
  QKING_ASSERT (object_p != NULL);
  QKING_ASSERT (ecma_gc_is_object_visited (object_p));

  bool traverse_properties = true;

  if (ecma_is_lexical_environment (object_p))
  {
    ecma_object_t *lex_env_p = ecma_get_lex_env_outer_reference (object_p);
    if (lex_env_p != NULL)
    {
      ecma_gc_set_object_visited (lex_env_p);
    }

    if (ecma_get_lex_env_type (object_p) != ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_object_t *binding_object_p = ecma_get_lex_env_binding_object (object_p);
      ecma_gc_set_object_visited (binding_object_p);

      traverse_properties = false;
    }
  }
  else
  {
    ecma_object_t *proto_p = ecma_get_object_prototype (object_p);
    if (proto_p != NULL)
    {
      ecma_gc_set_object_visited (proto_p);
    }

    switch (ecma_get_object_type (object_p))
    {
      case ECMA_OBJECT_TYPE_CLASS:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        switch (ext_object_p->u.class_prop.class_id)
        {
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
          case LIT_MAGIC_STRING_PROMISE_UL:
          {
            ecma_gc_mark_promise_object (ext_object_p);
            break;
          }
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN
          case LIT_MAGIC_STRING_MAP_UL:
          {
            ecma_gc_mark_map_object (ext_object_p);
            break;
          }
#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */
          default:
          {
            break;
          }
        }

        break;
      }
      case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        switch (ext_object_p->u.pseudo_array.type)
        {
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
          case ECMA_PSEUDO_ARRAY_TYPEDARRAY:
          case ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO:
          {
            ecma_gc_set_object_visited (ecma_typedarray_get_arraybuffer (object_p));
            break;
          }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
          default:
          {
            QKING_ASSERT (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS);

            ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                        ext_object_p->u.pseudo_array.u2.lex_env_cp);

            ecma_gc_set_object_visited (lex_env_p);
            break;
          }
        }

        break;
      }
      case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      {
        ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) object_p;

        ecma_object_t *target_func_obj_p;
        target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                             ext_function_p->u.bound_function.target_function);

        ecma_gc_set_object_visited (target_func_obj_p);

        ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;

        if (!ecma_is_value_integer_number (args_len_or_this))
        {
          if (ecma_is_value_object (args_len_or_this))
          {
            ecma_gc_set_object_visited (ecma_get_object_from_value (args_len_or_this));
          }
          break;
        }

        ecma_integer_value_t args_length = ecma_get_integer_from_value (args_len_or_this);
        ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

        QKING_ASSERT (args_length > 0);

        for (ecma_integer_value_t i = 0; i < args_length; i++)
        {
          if (ecma_is_value_object (args_p[i]))
          {
            ecma_gc_set_object_visited (ecma_get_object_from_value (args_p[i]));
          }
        }
        break;
      }
      case ECMA_OBJECT_TYPE_FUNCTION:
      {
        if (!ecma_get_object_is_builtin (object_p))
        {
          ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;

          ecma_gc_set_object_visited (ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                       ext_func_p->u.function.scope_cp));
          if (ext_func_p->u.function.closure_cp != ECMA_NULL_POINTER) {
              ecma_gc_mark_closure(object_p);
          }
        }
        break;
      }
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
      case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
      {
        ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) object_p;

        ecma_gc_set_object_visited (ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                               arrow_func_p->scope_cp));

        if (ecma_is_value_object (arrow_func_p->this_binding))
        {
          ecma_gc_set_object_visited (ecma_get_object_from_value (arrow_func_p->this_binding));
        }
        
        if (arrow_func_p->closure_cp != ECMA_NULL_POINTER) {
            ecma_gc_mark_closure(object_p);
        }
        break;
      }
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
      default:
      {
        break;
      }
    }
  }

  if (traverse_properties)
  {
    ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

    if (prop_iter_p != NULL && prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                      prop_iter_p->next_property_cp);
    }

    while (prop_iter_p != NULL)
    {
      QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

      ecma_gc_mark_property ((ecma_property_pair_t *) prop_iter_p, 0);
      ecma_gc_mark_property ((ecma_property_pair_t *) prop_iter_p, 1);

      prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                      prop_iter_p->next_property_cp);
    }
  }
    
#ifdef QKING_ENABLE_GC_DEBUG
    s_gc_mark = 0;
#endif

} /* ecma_gc_mark */

/**
 * Free the native handle/pointer by calling its free callback.
 */
static void
ecma_gc_free_native_pointer (ecma_property_t *property_p) /**< property */
{
  QKING_ASSERT (property_p != NULL);

  ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
  ecma_native_pointer_t *native_pointer_p;

  native_pointer_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_native_pointer_t,
                                                      value_p->value);

  if (native_pointer_p->info_p != NULL)
  {
    ecma_object_native_free_callback_t free_cb = native_pointer_p->info_p->free_cb;

    if (free_cb != NULL)
    {
      free_cb (native_pointer_p->data_p);
    }
  }

  jmem_heap_free_block (native_pointer_p, sizeof (ecma_native_pointer_t));
} /* ecma_gc_free_native_pointer */

/**
 * Free specified object.
 */
static void
ecma_gc_free_object (ecma_object_t *object_p) /**< object to free */
{
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_free_alive(ecma_make_object_value(object_p));
  printf("[gc][%d]=>[free]\n", ecma_make_object_value(object_p));
#endif
  QKING_ASSERT (object_p != NULL
                && !ecma_gc_is_object_visited (object_p)
                && object_p->type_flags_refs < ECMA_OBJECT_REF_ONE);

  bool obj_is_not_lex_env = !ecma_is_lexical_environment (object_p);

  if ((obj_is_not_lex_env
       || ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE))
  {
    ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

    if (prop_iter_p != NULL && prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      ecma_property_hashmap_free (object_p);
      prop_iter_p = ecma_get_property_list (object_p);
    }

    while (prop_iter_p != NULL)
    {
      QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

      /* Both cannot be deleted. */
      QKING_ASSERT (prop_iter_p->types[0] != ECMA_PROPERTY_TYPE_DELETED
                    || prop_iter_p->types[1] != ECMA_PROPERTY_TYPE_DELETED);

      ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

      for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
      {
        ecma_property_t *property_p = (ecma_property_t *) (prop_iter_p->types + i);
        jmem_cpointer_t name_cp = prop_pair_p->names_cp[i];

        /* Call the native's free callback. */
        if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_DIRECT_STRING_MAGIC
            && (name_cp == LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER))
        {
          ecma_gc_free_native_pointer (property_p);
        }

        if (prop_iter_p->types[i] != ECMA_PROPERTY_TYPE_DELETED)
        {
          ecma_free_property (object_p, name_cp, property_p);
        }
      }

      prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                      prop_iter_p->next_property_cp);

      ecma_dealloc_property_pair (prop_pair_p);
    }
  }

  QKING_ASSERT (QKING_CONTEXT (ecma_gc_objects_number) > 0);
  QKING_CONTEXT (ecma_gc_objects_number)--;

  if (obj_is_not_lex_env)
  {
    ecma_object_type_t object_type = ecma_get_object_type (object_p);

    size_t ext_object_size = sizeof (ecma_extended_object_t);

    if (ecma_get_object_is_builtin (object_p))
    {
      uint8_t length_and_bitset_size;

      if (object_type == ECMA_OBJECT_TYPE_CLASS
          || object_type == ECMA_OBJECT_TYPE_ARRAY)
      {
        ext_object_size = sizeof (ecma_extended_built_in_object_t);
        length_and_bitset_size = ((ecma_extended_built_in_object_t *) object_p)->built_in.length_and_bitset_size;
      }
      else
      {
        length_and_bitset_size = ((ecma_extended_object_t *) object_p)->u.built_in.length_and_bitset_size;
      }

      ext_object_size += (2 * sizeof (uint32_t)) * (length_and_bitset_size >> ECMA_BUILT_IN_BITSET_SHIFT);
    }

    if (object_type == ECMA_OBJECT_TYPE_CLASS)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      switch (ext_object_p->u.class_prop.class_id)
      {
        case LIT_MAGIC_STRING_STRING_UL:
        case LIT_MAGIC_STRING_NUMBER_UL:
        {
          ecma_free_value (ext_object_p->u.class_prop.u.value);
          break;
        }

        case LIT_MAGIC_STRING_DATE_UL:
        {
          ecma_number_t *num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t,
                                                                  ext_object_p->u.class_prop.u.value);
          ecma_dealloc_number (num_p);
          break;
        }

        case LIT_MAGIC_STRING_REGEXP_UL:
        {
          ecma_compiled_code_t *bytecode_p;
          bytecode_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (ecma_compiled_code_t,
                                                            ext_object_p->u.class_prop.u.value);

          if (bytecode_p != NULL)
          {
            ecma_bytecode_deref (bytecode_p);
          }
          break;
        }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
        case LIT_MAGIC_STRING_ARRAY_BUFFER_UL:
        {
          ecma_length_t arraybuffer_length = ext_object_p->u.class_prop.u.length;
          size_t size;

          if (ECMA_ARRAYBUFFER_HAS_EXTERNAL_MEMORY (ext_object_p))
          {
            size = sizeof (ecma_arraybuffer_external_info);

            /* Call external free callback if any. */
            ecma_arraybuffer_external_info *array_p = (ecma_arraybuffer_external_info *) ext_object_p;
            QKING_ASSERT (array_p != NULL);

            if (array_p->free_cb != NULL)
            {
              (array_p->free_cb) (array_p->buffer_p);
            }
          }
          else
          {
            size = sizeof (ecma_extended_object_t) + arraybuffer_length;
          }

          ecma_dealloc_extended_object (object_p, size);
          return;
        }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
        case LIT_MAGIC_STRING_PROMISE_UL:
        {
          ecma_free_value_if_not_object (ext_object_p->u.class_prop.u.value);
          ecma_free_values_collection (((ecma_promise_object_t *) object_p)->fulfill_reactions,
                                       ECMA_COLLECTION_NO_REF_OBJECTS);
          ecma_free_values_collection (((ecma_promise_object_t *) object_p)->reject_reactions,
                                       ECMA_COLLECTION_NO_REF_OBJECTS);
          ecma_dealloc_extended_object (object_p, sizeof (ecma_promise_object_t));
          return;
        }
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN
        case LIT_MAGIC_STRING_MAP_UL:
        {
          ecma_op_map_clear_map ((ecma_map_object_t *) object_p);
          ecma_dealloc_extended_object (object_p, sizeof (ecma_map_object_t));
          return;
        }
#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */
        default:
        {
          /* The undefined id represents an uninitialized class. */
          QKING_ASSERT (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_UNDEFINED
                        || ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_ARGUMENTS_UL
                        || ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_BOOLEAN_UL
                        || ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_ERROR_UL);
          break;
        }
      }

      ecma_dealloc_extended_object (object_p, ext_object_size);
      return;
    }

    if (ecma_get_object_is_builtin (object_p)
        || object_type == ECMA_OBJECT_TYPE_ARRAY
        || object_type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
    {
      ecma_dealloc_extended_object (object_p, ext_object_size);
      return;
    }

    if (object_type == ECMA_OBJECT_TYPE_FUNCTION)
    {
      /* Function with byte-code (not a built-in function). */
      ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;

#ifdef QKING_ENABLE_SNAPSHOT_EXEC
      if (ext_func_p->u.function.bytecode_cp != ECMA_NULL_POINTER)
      {
        ecma_bytecode_deref (ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t,
                                                              ext_func_p->u.function.bytecode_cp));
        ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t));
      }
      else
      {
        ecma_dealloc_extended_object (object_p, sizeof (ecma_static_function_t));
      }
#else /* !QKING_ENABLE_SNAPSHOT_EXEC */
      ecma_bytecode_deref (ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t,
                                                            ext_func_p->u.function.bytecode_cp));

      if (ext_func_p->u.function.closure_cp != ECMA_NULL_POINTER) {
          ecma_op_function_free_closure(object_p);
      }
      ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t));
#endif /* QKING_ENABLE_SNAPSHOT_EXEC */
      return;
    }

#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
    if (object_type == ECMA_OBJECT_TYPE_ARROW_FUNCTION)
    {
      ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) object_p;

      ecma_free_value_if_not_object (arrow_func_p->this_binding);

#ifdef QKING_ENABLE_SNAPSHOT_EXEC
      if (arrow_func_p->bytecode_cp != ECMA_NULL_POINTER)
      {
        ecma_bytecode_deref (ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                        arrow_func_p->bytecode_cp));
        ecma_dealloc_extended_object (object_p, sizeof (ecma_arrow_function_t));
      }
      else
      {
        ecma_dealloc_extended_object (object_p, sizeof (ecma_static_arrow_function_t));
      }
#else /* !QKING_ENABLE_SNAPSHOT_EXEC */
      ecma_bytecode_deref (ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                      arrow_func_p->bytecode_cp));
      if (arrow_func_p->closure_cp != ECMA_NULL_POINTER) {
          ecma_op_function_free_closure(object_p);
      }

      ecma_dealloc_extended_object (object_p, sizeof (ecma_arrow_function_t));
#endif /* QKING_ENABLE_SNAPSHOT_EXEC */
      return;
    }
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

    if (object_type == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      switch (ext_object_p->u.pseudo_array.type)
      {
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY:
        {
          ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t));
          return;
        }
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO:
        {
          ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_typedarray_object_t));
          return;
        }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
        default:
        {
          QKING_ASSERT (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS);

          ecma_length_t formal_params_number = ext_object_p->u.pseudo_array.u1.length;
          ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

          for (ecma_length_t i = 0; i < formal_params_number; i++)
          {
            if (arg_Literal_p[i] != ECMA_VALUE_EMPTY)
            {
              ecma_string_t *name_p = ecma_get_string_from_value (arg_Literal_p[i]);
              ecma_deref_ecma_string (name_p);
            }
          }

          size_t formal_params_size = formal_params_number * sizeof (ecma_value_t);
          ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t) + formal_params_size);
          return;
        }
      }
    }

    if (object_type == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
    {
      ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) object_p;

      ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;

      if (!ecma_is_value_integer_number (args_len_or_this))
      {
        ecma_free_value_if_not_object (args_len_or_this);
        ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t));
        return;
      }

      ecma_integer_value_t args_length = ecma_get_integer_from_value (args_len_or_this);
      ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

      for (ecma_integer_value_t i = 0; i < args_length; i++)
      {
        ecma_free_value_if_not_object (args_p[i]);
      }

      size_t args_size = ((size_t) args_length) * sizeof (ecma_value_t);
      ecma_dealloc_extended_object (object_p, sizeof (ecma_extended_object_t) + args_size);
      return;
    }
  }

  ecma_dealloc_object (object_p);
} /* ecma_gc_free_object */

#ifdef QKING_ENABLE_GC_DEBUG

void ecma_gc_info_ignore_start() {
    s_gc_start_ignore = true;
}

void ecma_gc_info_ignore_end() {
    s_gc_start_ignore = false;
}

bool ecma_gc_info_is_ignore(ecma_value_t var) {
    bool is_ignore = false;
    for (int i = 0; i < s_gc_ignore_count; i++) {
        if (s_gc_ignore[i] == var) {
            is_ignore = true;
            break;
        }
    }
    return is_ignore;
}

void ecma_gc_info_before_print() {
    s_gc_objects = (uint32_t)QKING_CONTEXT (ecma_gc_objects_number);
    printf("[gc][info][core][new]=>[%d][objs]\n", (uint32_t)QKING_CONTEXT (ecma_gc_new_objects));
#ifdef JMEM_STATS
    qking_heap_stats_t heap_stats;
    qking_get_memory_stats (&heap_stats);
    s_gc_allocated = heap_stats.allocated_bytes;
#endif
    ecma_object_t *white_gray_objects_p = QKING_CONTEXT (ecma_gc_objects_p);
    ecma_object_t *obj_iter_p = white_gray_objects_p;
    int object_visited_count = 0, object_unvisited_count = 0;
    while (obj_iter_p != NULL)
    {
        ecma_object_t *obj_next_p = ecma_gc_get_object_next (obj_iter_p);
        if (ecma_gc_info_is_ignore(ecma_make_object_value(obj_iter_p))) {
            object_unvisited_count++;
        }
        else if (ecma_gc_is_object_visited (obj_iter_p)) {
            uint16_t ref = obj_iter_p->type_flags_refs >> 6;
            if (ref > 0) {
                if (!ecma_is_lexical_environment(obj_iter_p) && ecma_get_object_is_builtin(obj_iter_p)) {
                    object_unvisited_count++;
                }
                else {
                    object_visited_count++;
                    ecma_gc_info_alive(ecma_make_object_value(obj_iter_p));
                    printf("[gc][info][print][%d]=>[ref][%d]\n", ecma_make_object_value(obj_iter_p), ref);
                }
            }
            else {
                object_unvisited_count++;
            }
        }
        else {
            object_unvisited_count++;
        }
        obj_iter_p = obj_next_p;
    }
    printf("[gc][info][print]=>[visited][%d][unvisited][%d]\n", object_visited_count, object_unvisited_count);
}

void ecma_gc_info_leak_print() {
    ecma_object_t *white_gray_objects_p = QKING_CONTEXT (ecma_gc_objects_p);
    ecma_object_t *obj_iter_p = white_gray_objects_p;
    while (obj_iter_p != NULL)
    {
        ecma_object_t *obj_next_p = ecma_gc_get_object_next (obj_iter_p);
        uint16_t ref = obj_iter_p->type_flags_refs >> 6;
        printf("[gc][info][core][leak][%d]=>[ref][%d]\n", ecma_make_object_value(obj_iter_p), ref);
        obj_iter_p = obj_next_p;
    }
}

void ecma_gc_info_after_print() {
    printf("[gc][info][core][obj]=>%d => %d free =>%d\n", s_gc_objects, (uint32_t)QKING_CONTEXT (ecma_gc_objects_number), s_gc_free_objects);
#ifdef JMEM_STATS
    qking_heap_stats_t heap_stats;
    qking_get_memory_stats (&heap_stats);
    printf("[gc][info][core][allocated]=>%zu => %zu bytes free =>%zu\n", s_gc_allocated, heap_stats.allocated_bytes, s_gc_allocated - heap_stats.allocated_bytes);
#endif
}

#endif

/**
 * Run garbage collection
 */
void
ecma_gc_run (jmem_free_unused_memory_severity_t severity) /**< gc severity */
{
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_info_before_print();
#endif
  QKING_CONTEXT (ecma_gc_new_objects) = 0;

  ecma_object_t *white_gray_objects_p = QKING_CONTEXT (ecma_gc_objects_p);
  ecma_object_t *black_objects_p = NULL;

  ecma_object_t *obj_iter_p = white_gray_objects_p;
  ecma_object_t *obj_prev_p = NULL;

  /* Move root objects (i.e. they have global or stack references) to the black list. */
  while (obj_iter_p != NULL)
  {
    ecma_object_t *obj_next_p = ecma_gc_get_object_next (obj_iter_p);

    QKING_ASSERT (obj_prev_p == NULL
                  || ecma_gc_get_object_next (obj_prev_p) == obj_iter_p);

    if (ecma_gc_is_object_visited (obj_iter_p))
    {
      /* Moving the object to list of marked objects. */
      if (QKING_LIKELY (obj_prev_p != NULL))
      {
        obj_prev_p->gc_next_cp = obj_iter_p->gc_next_cp;
      }
      else
      {
        white_gray_objects_p = obj_next_p;
      }

      ecma_gc_set_object_next (obj_iter_p, black_objects_p);
      black_objects_p = obj_iter_p;
    }
    else
    {
      obj_prev_p = obj_iter_p;
    }

    obj_iter_p = obj_next_p;
  }

  /* Mark root objects. */
  obj_iter_p = black_objects_p;
  while (obj_iter_p != NULL)
  {
    ecma_gc_mark (obj_iter_p);
    obj_iter_p = ecma_gc_get_object_next (obj_iter_p);
  }

  ecma_object_t *first_root_object_p = black_objects_p;

  /* Mark non-root objects. */
  bool marked_anything_during_current_iteration;

  do
  {
    marked_anything_during_current_iteration = false;

    obj_prev_p = NULL;
    obj_iter_p = white_gray_objects_p;

    while (obj_iter_p != NULL)
    {
      ecma_object_t *obj_next_p = ecma_gc_get_object_next (obj_iter_p);

      QKING_ASSERT (obj_prev_p == NULL
                    || ecma_gc_get_object_next (obj_prev_p) == obj_iter_p);

      if (ecma_gc_is_object_visited (obj_iter_p))
      {
        /* Moving the object to list of marked objects */
        if (QKING_LIKELY (obj_prev_p != NULL))
        {
          obj_prev_p->gc_next_cp = obj_iter_p->gc_next_cp;
        }
        else
        {
          white_gray_objects_p = obj_next_p;
        }

        ecma_gc_set_object_next (obj_iter_p, black_objects_p);
        black_objects_p = obj_iter_p;

        ecma_gc_mark (obj_iter_p);
        marked_anything_during_current_iteration = true;
      }
      else
      {
        obj_prev_p = obj_iter_p;
      }

      obj_iter_p = obj_next_p;
    }
  }
  while (marked_anything_during_current_iteration);

  /* Sweep objects that are currently unmarked. */
  obj_iter_p = white_gray_objects_p;
    
#ifdef QKING_ENABLE_GC_DEBUG
  s_gc_free_objects = 0;
#endif

  while (obj_iter_p != NULL)
  {
    ecma_object_t *obj_next_p = ecma_gc_get_object_next (obj_iter_p);

    QKING_ASSERT (!ecma_gc_is_object_visited (obj_iter_p));

    ecma_gc_free_object (obj_iter_p);
    obj_iter_p = obj_next_p;
      
#ifdef QKING_ENABLE_GC_DEBUG
    s_gc_free_objects++;
#endif
      
  }

  /* Reset the reference counter of non-root black objects. */
  obj_iter_p = black_objects_p;

  while (obj_iter_p != first_root_object_p)
  {
    /* The reference counter must be 1. */
    ecma_deref_object (obj_iter_p);
    QKING_ASSERT (obj_iter_p->type_flags_refs < ECMA_OBJECT_REF_ONE);

    obj_iter_p = ecma_gc_get_object_next (obj_iter_p);
  }

  if (severity == JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH)
  {
    obj_iter_p = black_objects_p;

    while (obj_iter_p != NULL)
    {
      if (!ecma_is_lexical_environment (obj_iter_p)
          || ecma_get_lex_env_type (obj_iter_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
      {
        ecma_property_header_t *prop_iter_p = ecma_get_property_list (obj_iter_p);

        if (prop_iter_p != NULL && prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
        {
          ecma_property_hashmap_free (obj_iter_p);
        }
      }

      obj_iter_p = ecma_gc_get_object_next (obj_iter_p);
    }
  }

  QKING_CONTEXT (ecma_gc_objects_p) = black_objects_p;

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
  /* Free RegExp bytecodes stored in cache */
  re_cache_gc_run ();
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
    
#ifdef QKING_ENABLE_GC_DEBUG
    ecma_gc_info_after_print();
#endif

} /* ecma_gc_run */

/**
 * Try to free some memory (depending on severity).
 */
void
ecma_free_unused_memory (jmem_free_unused_memory_severity_t severity) /**< severity of the request */
{
#ifdef QKING_DEBUGGER
  while ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
         && QKING_CONTEXT (debugger_byte_code_free_tail) != ECMA_NULL_POINTER)
  {
    /* Wait until all byte code is freed or the connection is aborted. */
    qking_debugger_receive (NULL);
  }
#endif /* QKING_DEBUGGER */

  if (severity == JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW)
  {
#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
    if (QKING_CONTEXT (ecma_prop_hashmap_alloc_state) > ECMA_PROP_HASHMAP_ALLOC_ON)
    {
      --QKING_CONTEXT (ecma_prop_hashmap_alloc_state);
    }
    QKING_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_HIGH_SEV_GC;
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

    /*
     * If there is enough newly allocated objects since last GC, probably it is worthwhile to start GC now.
     * Otherwise, probability to free sufficient space is considered to be low.
     */
    size_t new_objects_share = CONFIG_ECMA_GC_NEW_OBJECTS_SHARE_TO_START_GC;

    if (QKING_CONTEXT (ecma_gc_new_objects) * new_objects_share > QKING_CONTEXT (ecma_gc_objects_number))
    {
      ecma_gc_run (severity);
    }
  }
  else
  {
    QKING_ASSERT (severity == JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH);

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
    if (QKING_CONTEXT (status_flags) & ECMA_STATUS_HIGH_SEV_GC)
    {
      QKING_CONTEXT (ecma_prop_hashmap_alloc_state) = ECMA_PROP_HASHMAP_ALLOC_MAX;
    }
    else if (QKING_CONTEXT (ecma_prop_hashmap_alloc_state) < ECMA_PROP_HASHMAP_ALLOC_MAX)
    {
      ++QKING_CONTEXT (ecma_prop_hashmap_alloc_state);
      QKING_CONTEXT (status_flags) |= ECMA_STATUS_HIGH_SEV_GC;
    }
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

    /* Freeing as much memory as we currently can */
    ecma_gc_run (severity);
  }
} /* ecma_free_unused_memory */

/**
 * @}
 * @}
 */
