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

#ifndef QKING_PORT_H
#define QKING_PORT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "qking_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Error codes
 */
typedef enum {
  ERR_OUT_OF_MEMORY = 10,
  ERR_SYSCALL = 11,
  ERR_REF_COUNT_LIMIT = 12,
  ERR_DISABLED_BYTE_CODE = 13,
  ERR_FAILED_INTERNAL_ASSERTION = 120
} qking_fatal_code_t;

/**
 * Signal the port that qking experienced a fatal failure from which it cannot
 * recover.
 *
 * @param code gives the cause of the error.
 *
 * Note:
 *      Qking expects the function not to return.
 *
 * Example: a libc-based port may implement this with exit() or abort(), or
 * both.
 */
void qking_port_fatal(qking_fatal_code_t code);

/*
 *  I/O Port API
 */

/**
 * Qking log levels. The levels are in severity order
 * where the most serious levels come first.
 */
typedef enum {
  QKING_LOG_LEVEL_ERROR,   /**< the engine will terminate after the message is
                              printed */
  QKING_LOG_LEVEL_WARNING, /**< a request is aborted, but the engine continues
                              its operation */
  QKING_LOG_LEVEL_DEBUG,   /**< debug messages from the engine, low volume */
  QKING_LOG_LEVEL_TRACE /**< detailed info about engine internals, potentially
                           high volume */
} qking_log_level_t;

/**
 * Display or log a debug/error message. The function should implement a
 * printf-like interface, where the first argument specifies the log level and
 * the second argument specifies a format string on how to stringify the rest of
 * the parameter list.
 *
 * This function is only called with messages coming from the qking engine as
 * the result of some abnormal operation or describing its internal operations
 * (e.g., data structure dumps or tracing info).
 *
 * It should be the port that decides whether error and debug messages are
 * logged to the console, or saved to a database or to a file.
 *
 * Example: a libc-based port may implement this with vfprintf(stderr) or
 * vfprintf(logfile), or both, depending on log level.
 *
 * Note:
 *      This port function is called by qking-core when QKING_ENABLE_LOGGING is
 *      defined. It is also common practice though to use this function in
 *      application code.
 */
void QKING_ATTR_FORMAT(printf, 2, 3)
    qking_port_log(qking_log_level_t level, const char *format, ...);

/*
 * Date Port API
 */

/**
 * Get local time zone adjustment, in milliseconds, for the given timestamp.
 * The timestamp can be specified in either UTC or local time, depending on
 * the value of is_utc. Adding the value returned from this function to
 * a timestamp in UTC time should result in local time for the current time
 * zone, and subtracting it from a timestamp in local time should result in
 * UTC time.
 *
 * Ideally, this function should satisfy the stipulations applied to LocalTZA
 * in section 20.3.1.7 of the ECMAScript version 9.0 spec.
 *
 * See Also:
 *          ECMA-262 v9, 20.3.1.7
 *
 * Note:
 *      This port function is called by qking-core when
 *      CONFIG_DISABLE_DATE_BUILTIN is _not_ defined. Otherwise this function is
 *      not used.
 *
 * @param unix_ms The unix timestamp we want an offset for, given in
 *                millisecond precision (could be now, in the future,
 *                or in the past). As with all unix timestamps, 0 refers to
 *                1970-01-01, a day is exactly 86 400 000 milliseconds, and
 *                leap seconds cause the same second to occur twice.
 * @param is_utc Is the given timestamp in UTC time? If false, it is in local
 *               time.
 *
 * @return milliseconds between local time and UTC for the given timestamp,
 *         if available
 *.        0 if not available / we are in UTC.
 */
double qking_port_get_local_time_zone_adjustment(double unix_ms, bool is_utc);

/**
 * Get system time
 *
 * Note:
 *      This port function is called by qking-core when
 *      CONFIG_DISABLE_DATE_BUILTIN is _not_ defined. It is also common practice
 *      in application code to use this function for the initialization of the
 *      random number generator.
 *
 * @return milliseconds since Unix epoch
 */
double qking_port_get_current_time(void);

/**
 * Get the current context of the engine. Each port should provide its own
 * implementation of this interface.
 *
 * Note:
 *      This port function is called by qking-core when
 *      QKING_ENABLE_EXTERNAL_CONTEXT is defined. Otherwise this function is not
 *      used.
 *
 * @return the pointer to the engine context.
 */
struct qking_context_t *qking_port_get_current_context(void);

/**
 * Makes the process sleep for a given time.
 *
 * Note:
 *      This port function is called by qking-core when QKING_DEBUGGER is
 *      defined. Otherwise this function is not used.
 *
 * @param sleep_time milliseconds to sleep.
 */
void qking_port_sleep(uint32_t sleep_time);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKING_PORT_H */
