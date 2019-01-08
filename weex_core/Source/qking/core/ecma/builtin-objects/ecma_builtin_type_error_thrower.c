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
#include "ecma_alloc.h"
#include "ecma_builtins.h"
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_objects.h"
#include "ecma_try_catch_macro.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma_builtins_internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma_builtin_type_error_thrower.inc.h"
#define BUILTIN_UNDERSCORED_ID type_error_thrower
#include "ecma_builtin_internal_routines_template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup type_error_thrower ECMA [[ThrowTypeError]] object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in [[ThrowTypeError]] object
 *
 * See also:
 *          ECMA-262 v5, 13.2.3
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_type_error_thrower_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                               ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  /* The object should throw TypeError */
  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_builtin_type_error_thrower_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in [[ThrowTypeError]] object
 *
 * See also:
 *          ECMA-262 v5, 13.2.3
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_type_error_thrower_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  /* The object is not a constructor */
  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_builtin_type_error_thrower_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
