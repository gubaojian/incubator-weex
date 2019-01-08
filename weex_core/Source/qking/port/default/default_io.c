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
#include <stdarg.h>

#include "qking_port.h"
#include "qking_port_default.h"
#include "qking_debugger.h"

#ifndef DISABLE_EXTRA_API

/**
 * Actual log level
 */
static qking_log_level_t qking_port_default_log_level = QKING_LOG_LEVEL_ERROR;

#define QKING_PORT_DEFAULT_LOG_LEVEL qking_port_default_log_level

/**
 * Get the log level
 *
 * @return current log level
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
qking_log_level_t
qking_port_default_get_log_level (void)
{
  return qking_port_default_log_level;
} /* qking_port_default_get_log_level */

/**
 * Set the log level
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
void
qking_port_default_set_log_level (qking_log_level_t level) /**< log level */
{
  qking_port_default_log_level = level;
} /* qking_port_default_set_log_level */

#else /* DISABLE_EXTRA_API */
#define QKING_PORT_DEFAULT_LOG_LEVEL QKING_LOG_LEVEL_ERROR
#endif /* !DISABLE_EXTRA_API */

/**
 * Default implementation of qking_port_log. Prints log message to the standard
 * error with 'vfprintf' if message log level is less than or equal to the
 * current log level.
 *
 * If debugger support is enabled, printing happens first to an in-memory buffer,
 * which is then sent both to the standard error and to the debugger client.
 *
 * Note:
 *      Changing the log level from QKING_LOG_LEVEL_ERROR is only possible if
 *      the port implementation library is compiled without the
 *      DISABLE_EXTRA_API macro.
 */
void
qking_port_log (qking_log_level_t level, /**< message log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  if (level <= QKING_PORT_DEFAULT_LOG_LEVEL)
  {
    va_list args;
    va_start (args, format);
#ifdef QKING_DEBUGGER
    int length = vsnprintf (NULL, 0, format, args);
    va_end (args);
    va_start (args, format);

    char buffer[length + 1];
    vsnprintf (buffer, (size_t) length + 1, format, args);

    fprintf (stderr, "%s", buffer);
    qking_debugger_send_log (level, (qking_char_t *) buffer, (qking_size_t) length);
#else /* If qking-debugger isn't defined, libc is turned on */
    vfprintf (stderr, format, args);
#endif /* QKING_DEBUGGER */
    va_end (args);
  }
} /* qking_port_log */
