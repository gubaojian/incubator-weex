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
#ifndef ECMA_COMPILED_CODE_H
#define ECMA_COMPILED_CODE_H

#include "ecma_globals.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ecma_compiled_code_t *ecma_create_compiled_code(void);

/**
 * function state object creation operation.
 *
 *
 * @return ecma_function_state_t
 *         Returned function state
 */

ecma_compiled_function_state_t *ecma_create_compiled_function_state(void);

void ecma_compiled_func_state_create_instructions(
    ecma_compiled_function_state_t *function_state_p, size_t size);

void ecma_compiled_func_state_create_constants(
    ecma_compiled_function_state_t *function_state_p, size_t size);

void ecma_compiled_func_state_create_in_closure(
    ecma_compiled_function_state_t *function_state_p, size_t size);

void ecma_compiled_func_state_create_out_closure(
    ecma_compiled_function_state_t *function_state_p, size_t size);

void ecma_compiled_func_state_create_children_array(
    ecma_compiled_function_state_t *function_state_p, size_t size);

#ifndef QKING_NDEBUG

void ecma_finalize_compiled_function_state(
    ecma_compiled_function_state_t *function_state_p);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !ECMA_COMPILED_CODE_H */
