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
#include "handler.h"
#include "stdlib.h"

#ifndef QKING_NDEBUG

/**
 * Provide a 'print' implementation for scripts.
 *
 * The routine converts all of its arguments to strings and outputs them
 * char-by-char using qkingx_port_handler_print_char.
 *
 * The NUL character is output as "\u0000", other characters are output
 * bytewise.
 *
 * Note:
 *      This implementation does not use standard C `printf` to print its
 *      output. This allows more flexibility but also extends the core
 *      Qking engine port API. Applications that want to use
 *      `qkingx_handler_print` must ensure that their port implementation also
 *      provides `qkingx_port_handler_print_char`.
 *
 * @return undefined - if all arguments could be converted to strings,
 *         error - otherwise.
 */

qking_value_t qkingx_handler_print(
    const qking_value_t func_obj_val, /**< function object */
    const qking_value_t this_p,       /**< this arg */
    const qking_value_t args_p[],     /**< function arguments */
    const qking_length_t args_cnt)    /**< number of function arguments */
{
  return qking_print_log("[JSLog]=>",func_obj_val, this_p, args_p, args_cnt);
}
#endif
