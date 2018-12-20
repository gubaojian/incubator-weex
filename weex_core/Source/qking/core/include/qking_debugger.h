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

#ifndef QKING_DEBUGGER_H
#define QKING_DEBUGGER_H

#include "qking_core.h"
#include "qking_port.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \addtogroup qking-debugger Qking engine interface - Debugger feature
 * @{
 */

/**
 * Qking debugger protocol version.
 */
#define QKING_DEBUGGER_VERSION (7)

/**
 * Types for the client source wait and run method.
 */
typedef enum {
  QKING_DEBUGGER_SOURCE_RECEIVE_FAILED = 0, /**< source is not received */
  QKING_DEBUGGER_SOURCE_RECEIVED = 1,       /**< a source has been received */
  QKING_DEBUGGER_SOURCE_END = 2, /**< the end of the sources signal received */
  QKING_DEBUGGER_CONTEXT_RESET_RECEIVED, /**< the context reset request has been
                                            received */
} qking_debugger_wait_for_source_status_t;

/**
 * Callback for qking_debugger_wait_and_run_client_source
 *
 * The callback receives the resource name, source code and a user pointer.
 *
 * @return this value is passed back by
 * qking_debugger_wait_and_run_client_source
 */
typedef qking_value_t (*qking_debugger_wait_for_source_callback_t)(
    const qking_char_t *resource_name_p, size_t resource_name_size,
    const qking_char_t *source_p, size_t source_size, void *user_p);

/**
 * Engine debugger functions.
 */
bool qking_debugger_is_connected(void);
void qking_debugger_stop(void);
void qking_debugger_continue(void);
void qking_debugger_stop_at_breakpoint(bool enable_stop_at_breakpoint);
qking_debugger_wait_for_source_status_t qking_debugger_wait_for_client_source(
    qking_debugger_wait_for_source_callback_t callback_p, void *user_p,
    qking_value_t *return_value);
void qking_debugger_send_output(const qking_char_t *buffer,
                                qking_size_t str_size);
void qking_debugger_send_log(qking_log_level_t level,
                             const qking_char_t *buffer, qking_size_t str_size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKING_DEBUGGER_H */
