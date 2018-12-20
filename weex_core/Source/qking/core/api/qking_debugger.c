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
#include "jcontext.h"
#include "qking_internal.h"

/**
 * Checks whether the debugger is connected.
 *
 * @return true - if the debugger is connected
 *         false - otherwise
 */
bool
qking_debugger_is_connected (void)
{
#ifdef QKING_DEBUGGER
  return QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED;
#else /* !QKING_DEBUGGER */
  return false;
#endif /* QKING_DEBUGGER */
} /* qking_debugger_is_connected */

/**
 * Stop execution at the next available breakpoint.
 */
void
qking_debugger_stop (void)
{
#ifdef QKING_DEBUGGER
  if ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
      && !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_BREAKPOINT_MODE))
  {
    QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
    QKING_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* QKING_DEBUGGER */
} /* qking_debugger_stop */

/**
 * Continue execution.
 */
void
qking_debugger_continue (void)
{
#ifdef QKING_DEBUGGER
  if ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
      && !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_BREAKPOINT_MODE))
  {
    QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_VM_STOP);
    QKING_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* QKING_DEBUGGER */
} /* qking_debugger_continue */

/**
 * Sets whether the engine should stop at breakpoints.
 */
void
qking_debugger_stop_at_breakpoint (bool enable_stop_at_breakpoint) /**< enable/disable stop at breakpoint */
{
#ifdef QKING_DEBUGGER
  if (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED
      && !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_BREAKPOINT_MODE))
  {
    if (enable_stop_at_breakpoint)
    {
      QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_IGNORE);
    }
    else
    {
      QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_VM_IGNORE);
    }
  }
#else /* !QKING_DEBUGGER */
  QKING_UNUSED (enable_stop_at_breakpoint);
#endif /* QKING_DEBUGGER */
} /* qking_debugger_stop_at_breakpoint */

/**
 * Sets whether the engine should wait and run a source.
 *
 * @return enum QKING_DEBUGGER_SOURCE_RECEIVE_FAILED - if the source is not received
 *              QKING_DEBUGGER_SOURCE_RECEIVED - if a source code received
 *              QKING_DEBUGGER_SOURCE_END - the end of the source codes
 *              QKING_DEBUGGER_CONTEXT_RESET_RECEIVED - the end of the context
 */
qking_debugger_wait_for_source_status_t
qking_debugger_wait_for_client_source (qking_debugger_wait_for_source_callback_t callback_p, /**< callback function */
                                       void *user_p, /**< user pointer passed to the callback */
                                       qking_value_t *return_value) /**< [out] parse and run return value */
{
  *return_value = qking_create_undefined ();

#ifdef QKING_DEBUGGER
  if ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
      && !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_BREAKPOINT_MODE))
  {
    QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_CLIENT_SOURCE_MODE);
    qking_debugger_uint8_data_t *client_source_data_p = NULL;
    qking_debugger_wait_for_source_status_t ret_type = QKING_DEBUGGER_SOURCE_RECEIVE_FAILED;

    /* Notify the client about that the engine is waiting for a source. */
    qking_debugger_send_type (QKING_DEBUGGER_WAIT_FOR_SOURCE);

    while (true)
    {
      if (qking_debugger_receive (&client_source_data_p))
      {
        if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED))
        {
          break;
        }

        /* Stop executing the current context. */
        if ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONTEXT_RESET_MODE))
        {
          ret_type = QKING_DEBUGGER_CONTEXT_RESET_RECEIVED;
          QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_CONTEXT_RESET_MODE);
          break;
        }

        /* Stop waiting for a new source file. */
        if ((QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_NO_SOURCE))
        {
          ret_type = QKING_DEBUGGER_SOURCE_END;
          QKING_DEBUGGER_CLEAR_FLAGS (QKING_DEBUGGER_CLIENT_SOURCE_MODE);
          break;
        }

        /* The source arrived. */
        if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_SOURCE_MODE))
        {
          QKING_ASSERT (client_source_data_p != NULL);

          qking_char_t *resource_name_p = (qking_char_t *) (client_source_data_p + 1);
          size_t resource_name_size = strlen ((const char *) resource_name_p);

          *return_value = callback_p (resource_name_p,
                                      resource_name_size,
                                      resource_name_p + resource_name_size + 1,
                                      client_source_data_p->uint8_size - resource_name_size - 1,
                                      user_p);

          ret_type = QKING_DEBUGGER_SOURCE_RECEIVED;
          break;
        }
      }

      qking_debugger_transport_sleep ();
    }

    QKING_ASSERT (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CLIENT_SOURCE_MODE)
                  || !(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED));

    if (client_source_data_p != NULL)
    {
      /* The data may partly arrived. */
      jmem_heap_free_block (client_source_data_p,
                            client_source_data_p->uint8_size + sizeof (qking_debugger_uint8_data_t));
    }

    return ret_type;
  }

  return QKING_DEBUGGER_SOURCE_RECEIVE_FAILED;
#else /* !QKING_DEBUGGER */
  QKING_UNUSED (callback_p);
  QKING_UNUSED (user_p);

  return QKING_DEBUGGER_SOURCE_RECEIVE_FAILED;
#endif /* QKING_DEBUGGER */
} /* qking_debugger_wait_for_client_source */

/**
 * Send the output of the program to the debugger client.
 * Currently only sends print output.
 */
void
qking_debugger_send_output (const qking_char_t *buffer, /**< buffer */
                            qking_size_t str_size) /**< string size */
{
#ifdef QKING_DEBUGGER
  if (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
  {
    qking_debugger_send_string (QKING_DEBUGGER_OUTPUT_RESULT,
                                QKING_DEBUGGER_OUTPUT_OK,
                                (const uint8_t *) buffer,
                                sizeof (uint8_t) * str_size);
  }
#else /* !QKING_DEBUGGER */
  QKING_UNUSED (buffer);
  QKING_UNUSED (str_size);
#endif /* QKING_DEBUGGER */
} /* qking_debugger_send_output */

/**
 * Send the log of the program to the debugger client.
 */
void
qking_debugger_send_log (qking_log_level_t level, /**< level of the diagnostics message */
                         const qking_char_t *buffer, /**< buffer */
                         qking_size_t str_size) /**< string size */
{
#ifdef QKING_DEBUGGER
  if (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
  {
    qking_debugger_send_string (QKING_DEBUGGER_OUTPUT_RESULT,
                                (uint8_t) (level + 2),
                                (const uint8_t *) buffer,
                                sizeof (uint8_t) * str_size);
  }
#else /* !QKING_DEBUGGER */
  QKING_UNUSED (level);
  QKING_UNUSED (buffer);
  QKING_UNUSED (str_size);
#endif /* QKING_DEBUGGER */
} /* qking_debugger_send_log */
