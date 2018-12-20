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

#include "handler.h"
#include "qking_port.h"

#ifndef QKING_NDEBUG

/**
 * Hard assert for scripts. The routine calls qking_port_fatal on assertion failure.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
qking_value_t
qkingx_handler_assert_fatal (const qking_value_t func_obj_val, /**< function object */
                             const qking_value_t this_p, /**< this arg */
                             const qking_value_t args_p[], /**< function arguments */
                             const qking_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val; /* unused */
  (void) this_p; /* unused */

  if (args_cnt == 1
      && qking_value_is_boolean (args_p[0])
      && qking_get_boolean_value (args_p[0]))
  {
    return qking_create_boolean (true);
  }

  qking_port_log (QKING_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
  qking_port_fatal (ERR_FAILED_INTERNAL_ASSERTION);

  return qking_create_undefined();
} /* qkingx_handler_assert_fatal */

/**
 * Soft assert for scripts. The routine throws an error on assertion failure.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         error - otherwise.
 */
qking_value_t
qkingx_handler_assert_throw (const qking_value_t func_obj_val, /**< function object */
                             const qking_value_t this_p, /**< this arg */
                             const qking_value_t args_p[], /**< function arguments */
                             const qking_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val; /* unused */
  (void) this_p; /* unused */

  if (args_cnt == 1
      && qking_value_is_boolean (args_p[0])
      && qking_get_boolean_value (args_p[0]))
  {
    return qking_create_boolean (true);
  }

  return qking_create_error (QKING_ERROR_COMMON, "assertion failed");
} /* qkingx_handler_assert_throw */

/**
 * An alias to `qkingx_handler_assert_fatal`.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
qking_value_t
qkingx_handler_assert (const qking_value_t func_obj_val, /**< function object */
                       const qking_value_t this_p, /**< this arg */
                       const qking_value_t args_p[], /**< function arguments */
                       const qking_length_t args_cnt) /**< number of function arguments */
{
  return qkingx_handler_assert_fatal (func_obj_val, this_p, args_p, args_cnt);
} /* qkingx_handler_assert */

#endif
