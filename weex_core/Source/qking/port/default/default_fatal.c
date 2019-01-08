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
#include <stdlib.h>

#include "qking_port.h"
#include "qking_port_default.h"

#ifndef DISABLE_EXTRA_API

static bool abort_on_fail = false;

/**
 * Sets whether 'abort' should be called instead of 'exit' upon exiting with
 * non-zero exit code in the default implementation of qking_port_fatal.
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
void qking_port_default_set_abort_on_fail (bool flag) /**< new value of 'abort on fail' flag */
{
  abort_on_fail = flag;
} /* qking_port_default_set_abort_on_fail */

/**
 * Check whether 'abort' should be called instead of 'exit' upon exiting with
 * non-zero exit code in the default implementation of qking_port_fatal.
 *
 * @return true - if 'abort on fail' flag is set,
 *         false - otherwise
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
bool qking_port_default_is_abort_on_fail (void)
{
  return abort_on_fail;
} /* qking_port_default_is_abort_on_fail */

#endif /* !DISABLE_EXTRA_API */

static qking_port_default_fatal_t fatal_error_handler = NULL;

void qking_register_handler_fatal_error(qking_port_default_fatal_t fatal_handler) {
    fatal_error_handler = fatal_handler;
}

/**
 * Default implementation of qking_port_fatal. Calls 'abort' if exit code is
 * non-zero and "abort-on-fail" behaviour is enabled, 'exit' otherwise.
 *
 * Note:
 *      The "abort-on-fail" behaviour is only available if the port
 *      implementation library is compiled without the DISABLE_EXTRA_API macro.
 */
void qking_port_fatal (qking_fatal_code_t code) /**< cause of error */
{
  if (fatal_error_handler) {
    fatal_error_handler (code);
  }
#ifndef QKING_NDEBUG

#ifndef DISABLE_EXTRA_API
  if (code != 0
      && code != ERR_OUT_OF_MEMORY
      && abort_on_fail)
  {
    abort ();
  }
#endif /* !DISABLE_EXTRA_API */

  exit ((int) code);
    
#endif
} /* qking_port_fatal */
