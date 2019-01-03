/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "debugger.h"
#include "ecma_builtin_helpers.h"
#include "ecma_conversion.h"
#include "ecma_function_object.h"
#include "ecma_objects.h"
#include "jcontext.h"
#include "qking_port.h"
#include "lit_char_helpers.h"

#ifdef QKING_DEBUGGER

/**
 * Incoming message: next message of string data.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} qking_debugger_receive_uint8_data_part_t;

/**
 * The number of message types in the debugger should reflect the
 * debugger versioning.
 */
QKING_STATIC_ASSERT (QKING_DEBUGGER_MESSAGES_OUT_MAX_COUNT == 32
                     && QKING_DEBUGGER_MESSAGES_IN_MAX_COUNT == 21
                     && QKING_DEBUGGER_VERSION == 7,
                     debugger_version_correlates_to_message_type_count);

/**
 * Waiting for data from the client.
 */
#define QKING_DEBUGGER_RECEIVE_DATA_MODE \
  (QKING_DEBUGGER_BREAKPOINT_MODE | QKING_DEBUGGER_CLIENT_SOURCE_MODE)

/**
 * Type cast the debugger send buffer into a specific type.
 */
#define QKING_DEBUGGER_SEND_BUFFER_AS(type, name_p) \
  type *name_p = (type *) (QKING_CONTEXT (debugger_send_buffer_payload_p))

/**
 * Type cast the debugger receive buffer into a specific type.
 */
#define QKING_DEBUGGER_RECEIVE_BUFFER_AS(type, name_p) \
  type *name_p = ((type *) recv_buffer_p)

/**
 * Free all unreferenced byte code structures which
 * were not acknowledged by the debugger client.
 */
void
qking_debugger_free_unreferenced_byte_code (void)
{
  qking_debugger_byte_code_free_t *byte_code_free_p;

  byte_code_free_p = JMEM_CP_GET_POINTER (qking_debugger_byte_code_free_t,
                                          QKING_CONTEXT (debugger_byte_code_free_tail));

  while (byte_code_free_p != NULL)
  {
    qking_debugger_byte_code_free_t *prev_byte_code_free_p;
    prev_byte_code_free_p = JMEM_CP_GET_POINTER (qking_debugger_byte_code_free_t,
                                                 byte_code_free_p->prev_cp);

    jmem_heap_free_block (byte_code_free_p,
                          ((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);

    byte_code_free_p = prev_byte_code_free_p;
  }
} /* qking_debugger_free_unreferenced_byte_code */

/**
 * Send data over an active connection.
 *
 * @return true - if the data was sent successfully
 *         false - otherwise
 */
static bool
qking_debugger_send (size_t message_length) /**< message length in bytes */
{
  QKING_ASSERT (message_length <= QKING_CONTEXT (debugger_max_send_size));

  qking_debugger_transport_header_t *header_p = QKING_CONTEXT (debugger_transport_header_p);
  uint8_t *payload_p = QKING_CONTEXT (debugger_send_buffer_payload_p);

  return header_p->send (header_p, payload_p, message_length);
} /* qking_debugger_send */

/**
 * Send backtrace.
 */
static void
qking_debugger_send_backtrace (const uint8_t *recv_buffer_p) /**< pointer to the received data */
{
  QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_get_backtrace_t, get_backtrace_p);

  uint32_t min_depth;
  memcpy (&min_depth, get_backtrace_p->min_depth, sizeof (uint32_t));
  uint32_t max_depth;
  memcpy (&max_depth, get_backtrace_p->max_depth, sizeof (uint32_t));

  if (max_depth == 0)
  {
    max_depth = UINT32_MAX;
  }

  if (get_backtrace_p->get_total_frame_count != 0)
  {
    QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_backtrace_total_t, backtrace_total_p);
    backtrace_total_p->type = QKING_DEBUGGER_BACKTRACE_TOTAL;

    vm_frame_ctx_t *iter_frame_ctx_p = QKING_CONTEXT (vm_top_context_p);
    uint32_t frame_count = 0;
    while (iter_frame_ctx_p != NULL)
    {
      frame_count++;
      iter_frame_ctx_p = iter_frame_ctx_p->prev_context_p;
    }
    memcpy (backtrace_total_p->frame_count, &frame_count, sizeof (frame_count));

    qking_debugger_send (sizeof (qking_debugger_send_type_t) + sizeof (frame_count));
  }

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_backtrace_t, backtrace_p);

  backtrace_p->type = QKING_DEBUGGER_BACKTRACE;

  vm_frame_ctx_t *frame_ctx_p = QKING_CONTEXT (vm_top_context_p);

  size_t current_frame = 0;
  const size_t max_frame_count = QKING_DEBUGGER_SEND_MAX (qking_debugger_frame_t);
  const size_t max_message_size = QKING_DEBUGGER_SEND_SIZE (max_frame_count, qking_debugger_frame_t);

  if (min_depth <= max_depth)
  {
    uint32_t min_depth_offset = 0;

    while (frame_ctx_p != NULL && min_depth_offset < min_depth)
    {
      frame_ctx_p = frame_ctx_p->prev_context_p;
      min_depth_offset++;
    }

    while (frame_ctx_p != NULL && min_depth_offset++ < max_depth)
    {
      if (frame_ctx_p->bytecode_header_p->status_flags & ECMA_CODE_FLAGS_DEBUGGER_IGNORE)
      {
        frame_ctx_p = frame_ctx_p->prev_context_p;
        continue;
      }

      if (current_frame >= max_frame_count)
      {
        if (!qking_debugger_send (max_message_size))
        {
          return;
        }
        current_frame = 0;
      }

      qking_debugger_frame_t *frame_p = backtrace_p->frames + current_frame;

      jmem_cpointer_t byte_code_cp;
      JMEM_CP_SET_NON_NULL_POINTER (byte_code_cp, frame_ctx_p->bytecode_header_p);
      memcpy (frame_p->byte_code_cp, &byte_code_cp, sizeof (jmem_cpointer_t));

      uint32_t offset = (uint32_t) (frame_ctx_p->byte_code_p - (uint8_t *) frame_ctx_p->bytecode_header_p);
      memcpy (frame_p->offset, &offset, sizeof (uint32_t));

      frame_ctx_p = frame_ctx_p->prev_context_p;
      current_frame++;
    }
  }

  size_t message_size = current_frame * sizeof (qking_debugger_frame_t);

  backtrace_p->type = QKING_DEBUGGER_BACKTRACE_END;

  qking_debugger_send (sizeof (qking_debugger_send_type_t) + message_size);
} /* qking_debugger_send_backtrace */

/**
 * Send the scope chain types.
 */
static void
qking_debugger_send_scope_chain (void)
{
  vm_frame_ctx_t *iter_frame_ctx_p = QKING_CONTEXT (vm_top_context_p);

  const size_t max_byte_count = QKING_DEBUGGER_SEND_MAX (uint8_t);
  const size_t max_message_size = QKING_DEBUGGER_SEND_SIZE (max_byte_count, uint8_t);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_string_t, message_type_p);
  message_type_p->type = QKING_DEBUGGER_SCOPE_CHAIN;

  size_t buffer_pos = 0;
  bool next_func_is_local = true;
  ecma_object_t *lex_env_p = iter_frame_ctx_p->lex_env_p;

  while (true)
  {
    QKING_ASSERT (ecma_is_lexical_environment (lex_env_p));

    if (buffer_pos == max_byte_count)
    {
      if (!qking_debugger_send (max_message_size))
      {
        return;
      }

      buffer_pos = 0;
    }

    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      if ((lex_env_p->type_flags_refs & ECMA_OBJECT_FLAG_NON_CLOSURE) != 0)
      {
        message_type_p->string[buffer_pos++] = QKING_DEBUGGER_SCOPE_NON_CLOSURE;
      }
      else if (next_func_is_local)
      {
        message_type_p->string[buffer_pos++] = QKING_DEBUGGER_SCOPE_LOCAL;
        next_func_is_local = false;
      }
      else
      {
        message_type_p->string[buffer_pos++] = QKING_DEBUGGER_SCOPE_CLOSURE;
      }
    }
    else if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND)
    {
      if (ecma_get_lex_env_outer_reference (lex_env_p) == NULL)
      {
        message_type_p->string[buffer_pos++] = QKING_DEBUGGER_SCOPE_GLOBAL;
        break;
      }
      else
      {
        message_type_p->string[buffer_pos++] = QKING_DEBUGGER_SCOPE_WITH;
      }
    }

    lex_env_p = ecma_get_lex_env_outer_reference (lex_env_p);
  }

  message_type_p->type = QKING_DEBUGGER_SCOPE_CHAIN_END;

  qking_debugger_send (sizeof (qking_debugger_send_type_t) + buffer_pos);
} /* qking_debugger_send_scope_chain */

/**
 * Get type of the scope variable property.
 */
static qking_debugger_scope_variable_type_t
qking_debugger_get_variable_type (ecma_value_t value) /**< input ecma value */
{
  qking_debugger_scope_variable_type_t ret_value = QKING_DEBUGGER_VALUE_NONE;

  if (ecma_is_value_undefined (value))
  {
    ret_value = QKING_DEBUGGER_VALUE_UNDEFINED;
  }
  else if (ecma_is_value_null (value))
  {
    ret_value = QKING_DEBUGGER_VALUE_NULL;
  }
  else if (ecma_is_value_boolean (value))
  {
    ret_value = QKING_DEBUGGER_VALUE_BOOLEAN;
  }
  else if (ecma_is_value_number (value))
  {
    ret_value = QKING_DEBUGGER_VALUE_NUMBER;
  }
  else if (ecma_is_value_string (value))
  {
    ret_value = QKING_DEBUGGER_VALUE_STRING;
  }
  else
  {
    QKING_ASSERT (ecma_is_value_object (value));

    if (ecma_object_get_class_name (ecma_get_object_from_value (value)) == LIT_MAGIC_STRING_ARRAY_UL)
    {
      ret_value = QKING_DEBUGGER_VALUE_ARRAY;
    }
    else
    {
      ret_value = ecma_op_is_callable (value) ? QKING_DEBUGGER_VALUE_FUNCTION : QKING_DEBUGGER_VALUE_OBJECT;
    }
  }

  QKING_ASSERT (ret_value != QKING_DEBUGGER_VALUE_NONE);

  return ret_value;
} /* qking_debugger_get_variable_type */

/**
 * Helper function for qking_debugger_send_scope_variables.
 *
 * It will copies the given scope values type, length and value into the outgoing message string.
 *
 * @return true - if the copy was successfully
 *         false - otherwise
 */
static bool
qking_debugger_copy_variables_to_string_message (qking_debugger_scope_variable_type_t variable_type, /**< type */
                                                 ecma_string_t *value_str, /**< property name or value string */
                                                 qking_debugger_send_string_t *message_string_p, /**< msg pointer */
                                                 size_t *buffer_pos) /**< string data position of the message */
{
  const size_t max_byte_count = QKING_DEBUGGER_SEND_MAX (uint8_t);
  const size_t max_message_size = QKING_DEBUGGER_SEND_SIZE (max_byte_count, uint8_t);

  ECMA_STRING_TO_UTF8_STRING (value_str, str_buff, str_buff_size);

  size_t str_size = 0;
  size_t str_limit = 255;
  bool result = true;

  bool type_processed = false;

  while (true)
  {
    if (*buffer_pos == max_byte_count)
    {
      if (!qking_debugger_send (max_message_size))
      {
        result = false;
        break;
      }

      *buffer_pos = 0;
    }

    if (!type_processed)
    {
      if (variable_type != QKING_DEBUGGER_VALUE_NONE)
      {
        message_string_p->string[*buffer_pos] = variable_type;
        *buffer_pos += 1;
      }
      type_processed = true;
      continue;
    }

    if (variable_type == QKING_DEBUGGER_VALUE_FUNCTION)
    {
      str_size = 0; // do not copy function values
    }
    else
    {
      str_size = (str_buff_size > str_limit) ? str_limit : str_buff_size;
    }

    message_string_p->string[*buffer_pos] = (uint8_t) str_size;
    *buffer_pos += 1;
    break;
  }

  if (result)
  {
    size_t free_bytes = max_byte_count - *buffer_pos;
    const uint8_t *string_p = str_buff;

    while (str_size > free_bytes)
    {
      memcpy (message_string_p->string + *buffer_pos, string_p, free_bytes);

      if (!qking_debugger_send (max_message_size))
      {
        result = false;
        break;
      }

      string_p += free_bytes;
      str_size -= free_bytes;
      free_bytes = max_byte_count;
      *buffer_pos = 0;
    }

    if (result)
    {
      memcpy (message_string_p->string + *buffer_pos, string_p, str_size);
      *buffer_pos += str_size;
    }
  }

  ECMA_FINALIZE_UTF8_STRING (str_buff, str_buff_size);

  return result;
} /* qking_debugger_copy_variables_to_string_message */

/**
 * Send variables of the given scope chain level.
 */
static void
qking_debugger_send_scope_variables (const uint8_t *recv_buffer_p) /**< pointer to the received data */
{
  QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_get_scope_variables_t, get_scope_variables_p);

  uint32_t chain_index;
  memcpy (&chain_index, get_scope_variables_p->chain_index, sizeof (uint32_t));

  vm_frame_ctx_t *iter_frame_ctx_p = QKING_CONTEXT (vm_top_context_p);
  ecma_object_t *lex_env_p = iter_frame_ctx_p->lex_env_p;

  while (chain_index != 0)
  {
    lex_env_p = ecma_get_lex_env_outer_reference (lex_env_p);

    if (QKING_UNLIKELY (lex_env_p == NULL))
    {
      qking_debugger_send_type (QKING_DEBUGGER_SCOPE_VARIABLES_END);
      return;
    }

    if ((ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND)
        || (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE))
    {
      chain_index--;
    }
  }

  ecma_property_header_t *prop_iter_p;

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    prop_iter_p = ecma_get_property_list (lex_env_p);
  }
  else
  {
    QKING_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);
    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);
    prop_iter_p =  ecma_get_property_list (binding_obj_p);
  }

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_string_t, message_string_p);
  message_string_p->type = QKING_DEBUGGER_SCOPE_VARIABLES;

  size_t buffer_pos = 0;

  while (prop_iter_p != NULL)
  {
    QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if (ECMA_PROPERTY_IS_NAMED_PROPERTY (prop_iter_p->types[i]))
      {
        if (ECMA_PROPERTY_GET_NAME_TYPE (prop_iter_p->types[i]) == ECMA_DIRECT_STRING_MAGIC
            && prop_pair_p->names_cp[i] >= LIT_NON_INTERNAL_MAGIC_STRING__COUNT)
        {
          continue;
        }

        ecma_string_t *prop_name = ecma_string_from_property_name (prop_iter_p->types[i],
                                                                   prop_pair_p->names_cp[i]);

        if (!qking_debugger_copy_variables_to_string_message (QKING_DEBUGGER_VALUE_NONE,
                                                              prop_name,
                                                              message_string_p,
                                                              &buffer_pos))
        {
          ecma_deref_ecma_string (prop_name);
          return;
        }

        ecma_deref_ecma_string (prop_name);

        ecma_property_value_t prop_value_p = prop_pair_p->values[i];
        ecma_value_t property_value;

        qking_debugger_scope_variable_type_t variable_type = qking_debugger_get_variable_type (prop_value_p.value);

        if (variable_type == QKING_DEBUGGER_VALUE_OBJECT)
        {
          property_value = ecma_builtin_json_string_from_object (prop_value_p.value);
        }
        else
        {
          property_value = ecma_op_to_string (prop_value_p.value);
        }

        if (!qking_debugger_copy_variables_to_string_message (variable_type,
                                                              ecma_get_string_from_value (property_value),
                                                              message_string_p,
                                                              &buffer_pos))
        {
          ecma_free_value (property_value);
          return;
        }

        ecma_free_value (property_value);
      }
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  message_string_p->type = QKING_DEBUGGER_SCOPE_VARIABLES_END;
  qking_debugger_send (sizeof (qking_debugger_send_type_t) + buffer_pos);
} /* qking_debugger_send_scope_variables */

/**
 * Send result of evaluated expression or throw an error.
 *
 * @return true - if execution should be resumed
 *         false - otherwise
 */
static bool
qking_debugger_send_eval (const lit_utf8_byte_t *eval_string_p, /**< evaluated string */
                          size_t eval_string_size) /**< evaluated string size */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);
  QKING_ASSERT (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_VM_IGNORE));

  QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_IGNORE);
  ecma_value_t result = ecma_op_eval_chars_buffer (eval_string_p + 1, eval_string_size - 1, ECMA_PARSE_DIRECT_EVAL);
  QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_VM_IGNORE);

  if (!ECMA_IS_VALUE_ERROR (result))
  {
    if (eval_string_p[0] != QKING_DEBUGGER_EVAL_EVAL)
    {
      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_EXCEPTION_THROWN);
      QKING_CONTEXT (error_value) = result;

      /* Stop where the error is caught. */
      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
      QKING_CONTEXT (debugger_stop_context) = NULL;

      if (eval_string_p[0] == QKING_DEBUGGER_EVAL_THROW)
      {
        QKING_CONTEXT (status_flags) |= ECMA_STATUS_EXCEPTION;
      }
      else
      {
        QKING_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_EXCEPTION;
      }

      return true;
    }

    if (!ecma_is_value_string (result))
    {
      ecma_value_t to_string_value = ecma_op_to_string (result);
      ecma_free_value (result);
      result = to_string_value;
    }
  }

  ecma_value_t message = result;
  uint8_t type = QKING_DEBUGGER_EVAL_OK;

  if (ECMA_IS_VALUE_ERROR (result))
  {
    type = QKING_DEBUGGER_EVAL_ERROR;
    result = QKING_CONTEXT (error_value);

    if (ecma_is_value_object (result))
    {
      message = ecma_op_object_find (ecma_get_object_from_value (result),
                                     ecma_get_magic_string (LIT_MAGIC_STRING_MESSAGE));

      if (!ecma_is_value_string (message)
          || ecma_string_is_empty (ecma_get_string_from_value (message)))
      {
        ecma_free_value (message);
        lit_magic_string_id_t id = ecma_object_get_class_name (ecma_get_object_from_value (result));
        ecma_free_value (result);

        const lit_utf8_byte_t *string_p = lit_get_magic_string_utf8 (id);
        qking_debugger_send_string (QKING_DEBUGGER_EVAL_RESULT,
                                    type,
                                    string_p,
                                    strlen ((const char *) string_p));
        return false;
      }
    }
    else
    {
      /* Primitive type. */
      message = ecma_op_to_string (result);
      QKING_ASSERT (!ECMA_IS_VALUE_ERROR (message));
    }

    ecma_free_value (result);
  }

  ecma_string_t *string_p = ecma_get_string_from_value (message);

  ECMA_STRING_TO_UTF8_STRING (string_p, buffer_p, buffer_size);
  qking_debugger_send_string (QKING_DEBUGGER_EVAL_RESULT, type, buffer_p, buffer_size);
  ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);

  ecma_free_value (message);

  return false;
} /* qking_debugger_send_eval */

/**
 * Check received packet size.
 */
#define QKING_DEBUGGER_CHECK_PACKET_SIZE(type) \
  if (message_size != sizeof (type)) \
  { \
    QKING_ERROR_MSG ("Invalid message size\n"); \
    qking_debugger_transport_close (); \
    return false; \
  }

/**
 * Receive message from the client.
 *
 * @return true - if message is processed successfully
 *         false - otherwise
 */
static inline bool QKING_ATTR_ALWAYS_INLINE
qking_debugger_process_message (const uint8_t *recv_buffer_p, /**< pointer to the received data */
                                uint32_t message_size, /**< message size */
                                bool *resume_exec_p, /**< pointer to the resume exec flag */
                                uint8_t *expected_message_type_p, /**< message type */
                                qking_debugger_uint8_data_t **message_data_p) /**< custom message data */
{
  /* Process the received message. */

  if (recv_buffer_p[0] >= QKING_DEBUGGER_CONTINUE
      && !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_BREAKPOINT_MODE))
  {
    QKING_ERROR_MSG ("Message requires breakpoint mode\n");
    qking_debugger_transport_close ();
    return false;
  }

  if (*expected_message_type_p != 0)
  {
    QKING_ASSERT (*expected_message_type_p == QKING_DEBUGGER_EVAL_PART
                  || *expected_message_type_p == QKING_DEBUGGER_CLIENT_SOURCE_PART);

    qking_debugger_uint8_data_t *uint8_data_p = (qking_debugger_uint8_data_t *) *message_data_p;

    if (recv_buffer_p[0] != *expected_message_type_p)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (qking_debugger_uint8_data_t));
      QKING_ERROR_MSG ("Unexpected message\n");
      qking_debugger_transport_close ();
      return false;
    }

    QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_uint8_data_part_t, uint8_data_part_p);

    if (message_size < sizeof (qking_debugger_receive_uint8_data_part_t) + 1)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (qking_debugger_uint8_data_t));
      QKING_ERROR_MSG ("Invalid message size\n");
      qking_debugger_transport_close ();
      return false;
    }

    uint32_t expected_data = uint8_data_p->uint8_size - uint8_data_p->uint8_offset;

    message_size -= (uint32_t) sizeof (qking_debugger_receive_uint8_data_part_t);

    if (message_size > expected_data)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (qking_debugger_uint8_data_t));
      QKING_ERROR_MSG ("Invalid message size\n");
      qking_debugger_transport_close ();
      return false;
    }

    lit_utf8_byte_t *string_p = (lit_utf8_byte_t *) (uint8_data_p + 1);
    memcpy (string_p + uint8_data_p->uint8_offset,
            (lit_utf8_byte_t *) (uint8_data_part_p + 1),
            message_size);

    if (message_size < expected_data)
    {
      uint8_data_p->uint8_offset += message_size;
      return true;
    }

    bool result;

    if (*expected_message_type_p == QKING_DEBUGGER_EVAL_PART)
    {
      if (qking_debugger_send_eval (string_p, uint8_data_p->uint8_size))
      {
        *resume_exec_p = true;
      }
      result = (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED) != 0;
    }
    else
    {
      result = true;
      QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_CLIENT_SOURCE_MODE);
      *resume_exec_p = true;
    }

    *expected_message_type_p = 0;
    return result;
  }

  switch (recv_buffer_p[0])
  {
    case QKING_DEBUGGER_FREE_BYTE_CODE_CP:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_byte_code_cp_t);

      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_byte_code_cp_t, byte_code_p);

      jmem_cpointer_t byte_code_free_cp;
      memcpy (&byte_code_free_cp, byte_code_p->byte_code_cp, sizeof (jmem_cpointer_t));

      if (byte_code_free_cp != QKING_CONTEXT (debugger_byte_code_free_tail))
      {
        QKING_ERROR_MSG ("Invalid byte code free order\n");
        qking_debugger_transport_close ();
        return false;
      }

      qking_debugger_byte_code_free_t *byte_code_free_p;
      byte_code_free_p = JMEM_CP_GET_NON_NULL_POINTER (qking_debugger_byte_code_free_t,
                                                       byte_code_free_cp);

      if (byte_code_free_p->prev_cp != ECMA_NULL_POINTER)
      {
        QKING_CONTEXT (debugger_byte_code_free_tail) = byte_code_free_p->prev_cp;
      }
      else
      {
        QKING_CONTEXT (debugger_byte_code_free_head) = ECMA_NULL_POINTER;
        QKING_CONTEXT (debugger_byte_code_free_tail) = ECMA_NULL_POINTER;
      }

#ifdef JMEM_STATS
      jmem_stats_free_byte_code_bytes (((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);
#endif /* JMEM_STATS */

      jmem_heap_free_block (byte_code_free_p,
                            ((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);
      return true;
    }

    case QKING_DEBUGGER_UPDATE_BREAKPOINT:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_update_breakpoint_t);

      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_update_breakpoint_t, update_breakpoint_p);

      jmem_cpointer_t byte_code_cp;
      memcpy (&byte_code_cp, update_breakpoint_p->byte_code_cp, sizeof (jmem_cpointer_t));
      uint8_t *byte_code_p = JMEM_CP_GET_NON_NULL_POINTER (uint8_t, byte_code_cp);

      uint32_t offset;
      memcpy (&offset, update_breakpoint_p->offset, sizeof (uint32_t));
      byte_code_p += offset;

      QKING_ASSERT (*byte_code_p == ECMA_BREAKPOINT_ENABLED || *byte_code_p == ECMA_BREAKPOINT_DISABLED);

      *byte_code_p = update_breakpoint_p->is_set_breakpoint ? ECMA_BREAKPOINT_ENABLED : ECMA_BREAKPOINT_DISABLED;
      return true;
    }

    case QKING_DEBUGGER_MEMSTATS:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      qking_debugger_send_memstats ();
      return true;
    }

    case QKING_DEBUGGER_STOP:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
      QKING_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = false;
      return true;
    }

    case QKING_DEBUGGER_CONTINUE:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_VM_STOP);
      QKING_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = true;
      return true;
    }

    case QKING_DEBUGGER_STEP:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
      QKING_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = true;
      return true;
    }

    case QKING_DEBUGGER_NEXT:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
      QKING_CONTEXT (debugger_stop_context) = QKING_CONTEXT (vm_top_context_p);
      *resume_exec_p = true;
      return true;
    }

    case QKING_DEBUGGER_FINISH:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);

      /* This will point to the current context's parent (where the function was called)
       * and in case of NULL the result will the same as in case of STEP. */
      QKING_CONTEXT (debugger_stop_context) = QKING_CONTEXT (vm_top_context_p->prev_context_p);
      *resume_exec_p = true;
      return true;
    }

    case QKING_DEBUGGER_GET_BACKTRACE:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_get_backtrace_t);

      qking_debugger_send_backtrace (recv_buffer_p);
      return true;
    }

    case QKING_DEBUGGER_GET_SCOPE_CHAIN:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      qking_debugger_send_scope_chain ();

      return true;
    }

    case QKING_DEBUGGER_GET_SCOPE_VARIABLES:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_get_scope_variables_t);

      qking_debugger_send_scope_variables (recv_buffer_p);

      return true;
    }

    case QKING_DEBUGGER_EXCEPTION_CONFIG:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_exception_config_t);
      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_exception_config_t, exception_config_p);

      if (exception_config_p->enable == 0)
      {
        QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_IGNORE_EXCEPTION);
        QKING_DEBUG_MSG ("Stop at exception disabled\n");
      }
      else
      {
        QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_VM_IGNORE_EXCEPTION);
        QKING_DEBUG_MSG ("Stop at exception enabled\n");
      }

      return true;
    }

    case QKING_DEBUGGER_PARSER_CONFIG:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_parser_config_t);
      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_parser_config_t, parser_config_p);

      if (parser_config_p->enable_wait != 0)
      {
        QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_PARSER_WAIT);
        QKING_DEBUG_MSG ("Waiting after parsing enabled\n");
      }
      else
      {
        QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_PARSER_WAIT);
        QKING_DEBUG_MSG ("Waiting after parsing disabled\n");
      }

      return true;
    }

    case QKING_DEBUGGER_PARSER_RESUME:
    {
      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_PARSER_WAIT_MODE))
      {
        QKING_ERROR_MSG ("Not in parser wait mode\n");
        qking_debugger_transport_close ();
        return false;
      }

      QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_PARSER_WAIT_MODE);
      return true;
    }

    case QKING_DEBUGGER_EVAL:
    {
      if (message_size < sizeof (qking_debugger_receive_eval_first_t) + 1)
      {
        QKING_ERROR_MSG ("Invalid message size\n");
        qking_debugger_transport_close ();
        return false;
      }

      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_eval_first_t, eval_first_p);

      uint32_t eval_size;
      memcpy (&eval_size, eval_first_p->eval_size, sizeof (uint32_t));

      if (eval_size <= QKING_CONTEXT (debugger_max_receive_size) - sizeof (qking_debugger_receive_eval_first_t))
      {
        if (eval_size != message_size - sizeof (qking_debugger_receive_eval_first_t))
        {
          QKING_ERROR_MSG ("Invalid message size\n");
          qking_debugger_transport_close ();
          return false;
        }

        if (qking_debugger_send_eval ((lit_utf8_byte_t *) (eval_first_p + 1), eval_size))
        {
          *resume_exec_p = true;
        }

        return (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED) != 0;
      }

      qking_debugger_uint8_data_t *eval_uint8_data_p;
      size_t eval_data_size = sizeof (qking_debugger_uint8_data_t) + eval_size;

      eval_uint8_data_p = (qking_debugger_uint8_data_t *) jmem_heap_alloc_block (eval_data_size);

      eval_uint8_data_p->uint8_size = eval_size;
      eval_uint8_data_p->uint8_offset = (uint32_t) (message_size - sizeof (qking_debugger_receive_eval_first_t));

      lit_utf8_byte_t *eval_string_p = (lit_utf8_byte_t *) (eval_uint8_data_p + 1);
      memcpy (eval_string_p,
              (lit_utf8_byte_t *) (eval_first_p + 1),
              message_size - sizeof (qking_debugger_receive_eval_first_t));

      *message_data_p = eval_uint8_data_p;
      *expected_message_type_p = QKING_DEBUGGER_EVAL_PART;

      return true;
    }

    case QKING_DEBUGGER_CLIENT_SOURCE:
    {
      if (message_size <= sizeof (qking_debugger_receive_client_source_first_t))
      {
        QKING_ERROR_MSG ("Invalid message size\n");
        qking_debugger_transport_close ();
        return false;
      }

      if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        QKING_ERROR_MSG ("Not in client source mode\n");
        qking_debugger_transport_close ();
        return false;
      }

      QKING_DEBUGGER_RECEIVE_BUFFER_AS (qking_debugger_receive_client_source_first_t, client_source_first_p);

      uint32_t client_source_size;
      memcpy (&client_source_size, client_source_first_p->code_size, sizeof (uint32_t));

      uint32_t header_size = sizeof (qking_debugger_receive_client_source_first_t);

      if (client_source_size <= QKING_CONTEXT (debugger_max_receive_size) - header_size
          && client_source_size != message_size - header_size)
      {
        QKING_ERROR_MSG ("Invalid message size\n");
        qking_debugger_transport_close ();
        return false;
      }

      qking_debugger_uint8_data_t *client_source_data_p;
      size_t client_source_data_size = sizeof (qking_debugger_uint8_data_t) + client_source_size;

      client_source_data_p = (qking_debugger_uint8_data_t *) jmem_heap_alloc_block (client_source_data_size);

      client_source_data_p->uint8_size = client_source_size;
      client_source_data_p->uint8_offset = (uint32_t) (message_size
                                            - sizeof (qking_debugger_receive_client_source_first_t));

      lit_utf8_byte_t *client_source_string_p = (lit_utf8_byte_t *) (client_source_data_p + 1);
      memcpy (client_source_string_p,
              (lit_utf8_byte_t *) (client_source_first_p + 1),
              message_size - sizeof (qking_debugger_receive_client_source_first_t));

      *message_data_p = client_source_data_p;

      if (client_source_data_p->uint8_size != client_source_data_p->uint8_offset)
      {
        *expected_message_type_p = QKING_DEBUGGER_CLIENT_SOURCE_PART;
      }
      else
      {
        QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_CLIENT_SOURCE_MODE);
        *resume_exec_p = true;
      }
      return true;
    }

    case QKING_DEBUGGER_NO_MORE_SOURCES:
    {
      if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        QKING_ERROR_MSG ("Not in client source mode\n");
        qking_debugger_transport_close ();
        return false;
      }

      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_UPDATE_FLAGS (QKING_DEBUGGER_CLIENT_NO_SOURCE, QKING_DEBUGGER_CLIENT_SOURCE_MODE);

      *resume_exec_p = true;

      return true;
    }

    case QKING_DEBUGGER_CONTEXT_RESET:
    {
      if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        QKING_ERROR_MSG ("Not in client source mode\n");
        qking_debugger_transport_close ();
        return false;
      }

      QKING_DEBUGGER_CHECK_PACKET_SIZE (qking_debugger_receive_type_t);

      QKING_DEBUGGER_UPDATE_FLAGS (QKING_DEBUGGER_CONTEXT_RESET_MODE, QKING_DEBUGGER_CLIENT_SOURCE_MODE);

      *resume_exec_p = true;

      return true;
    }

    default:
    {
      QKING_ERROR_MSG ("Unexpected message.");
      qking_debugger_transport_close ();
      return false;
    }
  }
} /* qking_debugger_process_message */

/**
 * Receive message from the client.
 *
 * Note:
 *   If the function returns with true, the value of
 *   QKING_DEBUGGER_VM_STOP flag should be ignored.
 *
 * @return true - if execution should be resumed,
 *         false - otherwise
 */
bool
qking_debugger_receive (qking_debugger_uint8_data_t **message_data_p) /**< [out] data received from client */
{
  QKING_ASSERT (qking_debugger_transport_is_connected ());

  QKING_ASSERT (message_data_p != NULL ? !!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_RECEIVE_DATA_MODE)
                                       : !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_RECEIVE_DATA_MODE));

  QKING_CONTEXT (debugger_message_delay) = QKING_DEBUGGER_MESSAGE_FREQUENCY;

  bool resume_exec = false;
  uint8_t expected_message_type = 0;

  while (true)
  {
    qking_debugger_transport_receive_context_t context;
    if (!qking_debugger_transport_receive (&context))
    {
      QKING_ASSERT (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED));
      return true;
    }

    if (context.message_p == NULL)
    {
      if (expected_message_type != 0)
      {
        qking_debugger_transport_sleep ();
        continue;
      }

      return resume_exec;
    }

    /* Only datagram packets are supported. */
    QKING_ASSERT (context.message_total_length > 0);

    /* The qking_debugger_process_message function is inlined
     * so passing these arguments is essentially free. */
    if (!qking_debugger_process_message (context.message_p,
                                         (uint32_t) context.message_length,
                                         &resume_exec,
                                         &expected_message_type,
                                         message_data_p))
    {
      QKING_ASSERT (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED));
      return true;
    }

    qking_debugger_transport_receive_completed (&context);
  }
} /* qking_debugger_receive */


#undef QKING_DEBUGGER_CHECK_PACKET_SIZE

/**
 * Tell the client that a breakpoint has been hit and wait for further debugger commands.
 */
void
qking_debugger_breakpoint_hit (uint8_t message_type) /**< message type */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_breakpoint_hit_t, breakpoint_hit_p);

  breakpoint_hit_p->type = message_type;

  vm_frame_ctx_t *frame_ctx_p = QKING_CONTEXT (vm_top_context_p);

  jmem_cpointer_t byte_code_header_cp;
  JMEM_CP_SET_NON_NULL_POINTER (byte_code_header_cp, frame_ctx_p->bytecode_header_p);
  memcpy (breakpoint_hit_p->byte_code_cp, &byte_code_header_cp, sizeof (jmem_cpointer_t));

  uint32_t offset = (uint32_t) (frame_ctx_p->byte_code_p - (uint8_t *) frame_ctx_p->bytecode_header_p);
  memcpy (breakpoint_hit_p->offset, &offset, sizeof (uint32_t));

  if (!qking_debugger_send (sizeof (qking_debugger_send_breakpoint_hit_t)))
  {
    return;
  }

  QKING_DEBUGGER_UPDATE_FLAGS (QKING_DEBUGGER_BREAKPOINT_MODE, QKING_DEBUGGER_VM_EXCEPTION_THROWN);

  qking_debugger_uint8_data_t *uint8_data = NULL;

  while (!qking_debugger_receive (&uint8_data))
  {
    qking_debugger_transport_sleep ();
  }

  if (uint8_data != NULL)
  {
    jmem_heap_free_block (uint8_data,
                          uint8_data->uint8_size + sizeof (qking_debugger_uint8_data_t));
  }

  QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_BREAKPOINT_MODE);

  QKING_CONTEXT (debugger_message_delay) = QKING_DEBUGGER_MESSAGE_FREQUENCY;
} /* qking_debugger_breakpoint_hit */

/**
 * Send the type signal to the client.
 */
void
qking_debugger_send_type (qking_debugger_header_type_t type) /**< message type */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_type_t, message_type_p);

  message_type_p->type = (uint8_t) type;

  qking_debugger_send (sizeof (qking_debugger_send_type_t));
} /* qking_debugger_send_type */


/**
 * Send the type signal to the client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
qking_debugger_send_configuration (uint8_t max_message_size) /**< maximum message size */
{
  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_configuration_t, configuration_p);

  /* Helper structure for endianness check. */
  union
  {
    uint16_t uint16_value; /**< a 16-bit value */
    uint8_t uint8_value[2]; /**< lower and upper byte of a 16-bit value */
  } endian_data;

  endian_data.uint16_value = 1;

  configuration_p->type = QKING_DEBUGGER_CONFIGURATION;
  configuration_p->configuration = 0;

  if (endian_data.uint8_value[0] == 1)
  {
    configuration_p->configuration |= (uint8_t) QKING_DEBUGGER_LITTLE_ENDIAN;
  }

  uint32_t version = QKING_DEBUGGER_VERSION;
  memcpy (configuration_p->version, &version, sizeof (uint32_t));

  configuration_p->max_message_size = max_message_size;
  configuration_p->cpointer_size = sizeof (jmem_cpointer_t);

  return qking_debugger_send (sizeof (qking_debugger_send_configuration_t));
} /* qking_debugger_send_configuration */

/**
 * Send raw data to the debugger client.
 */
void
qking_debugger_send_data (qking_debugger_header_type_t type, /**< message type */
                          const void *data, /**< raw data */
                          size_t size) /**< size of data */
{
  QKING_ASSERT (size <= QKING_DEBUGGER_SEND_MAX (uint8_t));

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_type_t, message_type_p);

  message_type_p->type = (uint8_t) type;
  memcpy (message_type_p + 1, data, size);

  qking_debugger_send (sizeof (qking_debugger_send_type_t) + size);
} /* qking_debugger_send_data */

/**
 * Send string to the debugger client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
qking_debugger_send_string (uint8_t message_type, /**< message type */
                            uint8_t sub_type, /**< subtype of the string */
                            const uint8_t *string_p, /**< string data */
                            size_t string_length) /**< length of string */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  const size_t max_byte_count = QKING_DEBUGGER_SEND_MAX (uint8_t);
  const size_t max_message_size = QKING_DEBUGGER_SEND_SIZE (max_byte_count, uint8_t);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_string_t, message_string_p);

  message_string_p->type = message_type;

  if (sub_type != QKING_DEBUGGER_NO_SUBTYPE)
  {
    string_length += 1;
  }

  while (string_length > max_byte_count)
  {
    memcpy (message_string_p->string, string_p, max_byte_count);

    if (!qking_debugger_send (max_message_size))
    {
      return false;
    }

    string_length -= max_byte_count;
    string_p += max_byte_count;
  }

  message_string_p->type = (uint8_t) (message_type + 1);

  if (sub_type != QKING_DEBUGGER_NO_SUBTYPE)
  {
    memcpy (message_string_p->string, string_p, string_length - 1);
    message_string_p->string[string_length - 1] = sub_type;
  }
  else
  {
    memcpy (message_string_p->string, string_p, string_length);
  }

  return qking_debugger_send (sizeof (qking_debugger_send_type_t) + string_length);
} /* qking_debugger_send_string */

/**
 * Send the function compressed pointer to the debugger client.
 *
 * @return true - if the data was sent successfully to the debugger client,
 *         false - otherwise
 */
bool
qking_debugger_send_function_cp (qking_debugger_header_type_t type, /**< message type */
                                 ecma_compiled_code_t *compiled_code_p) /**< byte code pointer */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_byte_code_cp_t, byte_code_cp_p);

  byte_code_cp_p->type = (uint8_t) type;

  jmem_cpointer_t compiled_code_cp;
  JMEM_CP_SET_NON_NULL_POINTER (compiled_code_cp, compiled_code_p);
  memcpy (byte_code_cp_p->byte_code_cp, &compiled_code_cp, sizeof (jmem_cpointer_t));

  return qking_debugger_send (sizeof (qking_debugger_send_byte_code_cp_t));
} /* qking_debugger_send_function_cp */

/**
 * Send function data to the debugger client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
qking_debugger_send_parse_function (uint32_t line, /**< line */
                                    uint32_t column) /**< column */
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_parse_function_t, message_parse_function_p);

  message_parse_function_p->type = QKING_DEBUGGER_PARSE_FUNCTION;
  memcpy (message_parse_function_p->line, &line, sizeof (uint32_t));
  memcpy (message_parse_function_p->column, &column, sizeof (uint32_t));

  return qking_debugger_send (sizeof (qking_debugger_send_parse_function_t));
} /* qking_debugger_send_parse_function */

/**
 * Send memory statistics to the debugger client.
 */
void
qking_debugger_send_memstats (void)
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  QKING_DEBUGGER_SEND_BUFFER_AS (qking_debugger_send_memstats_t, memstats_p);

  memstats_p->type = QKING_DEBUGGER_MEMSTATS_RECEIVE;

#ifdef JMEM_STATS /* if jmem_stats are defined */
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  uint32_t allocated_bytes = (uint32_t) heap_stats->allocated_bytes;
  memcpy (memstats_p->allocated_bytes, &allocated_bytes, sizeof (uint32_t));
  uint32_t byte_code_bytes = (uint32_t) heap_stats->byte_code_bytes;
  memcpy (memstats_p->byte_code_bytes, &byte_code_bytes, sizeof (uint32_t));
  uint32_t string_bytes = (uint32_t) heap_stats->string_bytes;
  memcpy (memstats_p->string_bytes, &string_bytes, sizeof (uint32_t));
  uint32_t object_bytes = (uint32_t) heap_stats->object_bytes;
  memcpy (memstats_p->object_bytes, &object_bytes, sizeof (uint32_t));
  uint32_t property_bytes = (uint32_t) heap_stats->property_bytes;
  memcpy (memstats_p->property_bytes, &property_bytes, sizeof (uint32_t));
#else /* if not, just put zeros */
  memset (memstats_p->allocated_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->byte_code_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->string_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->object_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->property_bytes, 0, sizeof (uint32_t));
#endif

  qking_debugger_send (sizeof (qking_debugger_send_memstats_t));
} /* qking_debugger_send_memstats */

/*
 * Converts an standard error into a string.
 *
 * @return standard error string
 */
static ecma_string_t *
qking_debugger_exception_object_to_string (ecma_value_t exception_obj_value) /**< exception object */
{
  ecma_object_t *object_p = ecma_get_object_from_value (exception_obj_value);

  ecma_object_t *prototype_p = ecma_get_object_prototype (object_p);

  if (prototype_p == NULL
      || ecma_get_object_type (prototype_p) != ECMA_OBJECT_TYPE_GENERAL
      || !ecma_get_object_is_builtin (prototype_p))
  {
    return NULL;
  }

  lit_magic_string_id_t string_id;

  switch (((ecma_extended_object_t *) prototype_p)->u.built_in.id)
  {
#ifndef CONFIG_DISABLE_ERROR_BUILTINS
    case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_EVAL_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_RANGE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_REFERENCE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_SYNTAX_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_TYPE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_URI_ERROR_UL;
      break;
    }
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */
    case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_ERROR_UL;
      break;
    }
    default:
    {
      return NULL;
    }
  }

  lit_utf8_size_t size = lit_get_magic_string_size (string_id);
  QKING_ASSERT (size <= 14);

  lit_utf8_byte_t data[16];
  memcpy (data, lit_get_magic_string_utf8 (string_id), size);

  ecma_property_t *property_p;
  property_p = ecma_find_named_property (ecma_get_object_from_value (exception_obj_value),
                                         ecma_get_magic_string (LIT_MAGIC_STRING_MESSAGE));

  if (property_p == NULL
      || ECMA_PROPERTY_GET_TYPE (*property_p) != ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    return ecma_new_ecma_string_from_utf8 (data, size);
  }

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  if (!ecma_is_value_string (prop_value_p->value))
  {
    return ecma_new_ecma_string_from_utf8 (data, size);
  }

  data[size] = LIT_CHAR_COLON;
  data[size + 1] = LIT_CHAR_SP;

  return ecma_concat_ecma_strings (ecma_new_ecma_string_from_utf8 (data, size + 2),
                                   ecma_get_string_from_value (prop_value_p->value));
} /* qking_debugger_exception_object_to_string */

/**
 * Send string representation of exception to the client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
qking_debugger_send_exception_string (void)
{
  ecma_string_t *string_p = NULL;

  ecma_value_t exception_value = QKING_CONTEXT (error_value);

  if (ecma_is_value_object (exception_value))
  {
    string_p = qking_debugger_exception_object_to_string (exception_value);

    if (string_p == NULL)
    {
      string_p = ecma_get_string_from_value (ecma_builtin_helper_object_to_string (exception_value));
    }
  }
  else if (ecma_is_value_string (exception_value))
  {
    string_p = ecma_get_string_from_value (exception_value);
    ecma_ref_ecma_string (string_p);
  }
  else
  {
    exception_value = ecma_op_to_string (exception_value);
    string_p = ecma_get_string_from_value (exception_value);
  }

  ECMA_STRING_TO_UTF8_STRING (string_p, string_data_p, string_size);

  bool result = qking_debugger_send_string (QKING_DEBUGGER_EXCEPTION_STR,
                                            QKING_DEBUGGER_NO_SUBTYPE,
                                            string_data_p,
                                            string_size);

  ECMA_FINALIZE_UTF8_STRING (string_data_p, string_size);

  ecma_deref_ecma_string (string_p);
  return result;
} /* qking_debugger_send_exception_string */

#endif /* QKING_DEBUGGER */