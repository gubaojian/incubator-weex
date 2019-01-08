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
#include "ecma_builtins.h"
#include "ecma_exceptions.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_typedarray_object.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64

#define ECMA_BUILTINS_INTERNAL
#include "ecma_builtins_internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma_builtin_float64array.inc.h"
#define BUILTIN_UNDERSCORED_ID float64array
#include "ecma_builtin_internal_routines_template.inc.h"

#include "ecma_builtin_typedarray_helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup float64array ECMA Float64Array object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of Float64Array
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_float64array_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                         ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("Float64Array cannot be directly called"));
} /* ecma_builtin_float64array_dispatch_call */

/**
 * Handle calling [[Construct]] of Float64Array
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_float64array_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                              ecma_length_t arguments_list_len) /**< number of arguments */
{
  return ecma_typedarray_helper_dispatch_construct (arguments_list_p, arguments_list_len,
                                                    ECMA_BUILTIN_ID_FLOAT64ARRAY);
} /* ecma_builtin_float64array_dispatch_construct */

/**
  * @}
  * @}
  * @}
  */

#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
