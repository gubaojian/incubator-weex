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

/**
 * Register a JavaScript function in the global object.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful,
 *         error - otherwise.
 */
qking_value_t
qkingx_handler_register_global (const qking_char_t *name_p, /**< name of the function */
                                qking_external_handler_t handler_p) /**< function callback */
{
  qking_value_t global_obj_val = qking_get_global_object ();
  qking_value_t function_name_val = qking_create_string (name_p);
  qking_value_t function_val = qking_create_external_function (handler_p);

  qking_value_t result_val = qking_set_property (global_obj_val, function_name_val, function_val);

  qking_release_value (function_val);
  qking_release_value (function_name_val);
  qking_release_value (global_obj_val);

  return result_val;
} /* qkingx_handler_register_global */


qking_value_t
qkingx_variable_register_global (const qking_char_t *name_p, /**< name of the variable */
                                qking_value_t var) /**< variable */
{
    qking_value_t global_obj_val = qking_get_global_object ();
    qking_value_t name_val = qking_create_string (name_p);
    
    qking_value_t result_val = qking_set_property (global_obj_val, name_val, var);
    
    qking_release_value (name_val);
    qking_release_value (global_obj_val);
    
    return result_val;
} /* qkingx_handler_register_global */
