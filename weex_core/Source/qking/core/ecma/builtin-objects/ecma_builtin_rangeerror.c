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

#include "ecma_alloc.h"
#include "ecma_builtins.h"
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_builtin_helpers.h"
#include "ecma_objects.h"
#include "ecma_try_catch_macro.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_ERROR_BUILTINS

#define ECMA_BUILTINS_INTERNAL
#include "ecma_builtins_internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma_builtin_rangeerror.inc.h"
#define BUILTIN_UNDERSCORED_ID range_error
#include "ecma_builtin_internal_routines_template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup rangeerror ECMA RangeError object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in RangeError object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_range_error_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  return ecma_builtin_helper_error_dispatch_call (ECMA_ERROR_RANGE, arguments_list_p, arguments_list_len);
} /* ecma_builtin_range_error_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in RangeError object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_range_error_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                             ecma_length_t arguments_list_len) /**< number of arguments */
{
  return ecma_builtin_range_error_dispatch_call (arguments_list_p, arguments_list_len);
} /* ecma_builtin_range_error_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */
