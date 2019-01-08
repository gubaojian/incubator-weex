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
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_gc.h"
#include "ecma_function_object.h"
#include "ecma_lex_env.h"
#include "ecma_try_catch_macro.h"
#include "ecma_compiled_code.h"
#include "lit_magic_strings.h"

#ifdef QKING_ENABLE_LINE_INFO
#include "jcontext.h"
#endif /* QKING_ENABLE_LINE_INFO */

#define ECMA_BUILTINS_INTERNAL
#include "ecma_builtins_internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma_builtin_function.inc.h"
#define BUILTIN_UNDERSCORED_ID function
#include "ecma_builtin_internal_routines_template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup function ECMA Function object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Function object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                     ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_builtin_function_dispatch_construct (arguments_list_p, arguments_list_len);
} /* ecma_builtin_function_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Function object
 *
 * See also:
 *          ECMA-262 v5, 15.3.
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                          ecma_length_t arguments_list_len) /**< number of arguments */
{
    QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);
    if (arguments_list_len) {
        return ecma_raise_syntax_error ("unsupporting function construct");
    }
    ecma_compiled_code_t *bytecode_data_p = ecma_create_compiled_code();
    ecma_object_t *func_obj_p = ecma_op_create_function_object (ecma_get_global_environment (),
                                                                bytecode_data_p);
    return ecma_make_object_value (func_obj_p);
} /* ecma_builtin_function_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
