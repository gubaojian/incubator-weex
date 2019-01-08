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
#ifndef VM_H
#define VM_H

#include "ecma_globals.h"
#include "jrt.h"
#include "vm_defines.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Non-recursive vm_loop: the vm_loop can be suspended
 * to execute a call /construct operation. These return
 * types of the vm_loop tells whether a call operation
 * is in progress or the vm_loop is finished.
 */
typedef enum {
  VM_NO_EXEC_OP,     /**< do nothing */
  VM_EXEC_CALL,      /**< invoke a function */
  VM_EXEC_CONSTRUCT, /**< construct a new object */
} vm_call_operation;

ecma_value_t vm_run(const ecma_compiled_code_t *bytecode_header_p,
                    ecma_value_t this_func_value,
                    ecma_value_t this_binding_value, ecma_object_t *lex_env_p,
                    const ecma_value_t *arg_list_p, ecma_length_t argc);

bool vm_is_strict_mode(void);

#define VM_CREATE_CONTEXT(type, end_offset) \
  ((ecma_value_t)((type) | (end_offset) << 4))
#define VM_GET_CONTEXT_TYPE(value) ((vm_stack_context_type_t)((value)&0xf))
#define VM_GET_CONTEXT_END(value) ((value) >> 4)

typedef enum {
  VM_CONTEXT_FINALLY_JUMP,   /**< finally context with a jump */
  VM_CONTEXT_FINALLY_THROW,  /**< finally context with a throw */
  VM_CONTEXT_FINALLY_RETURN, /**< finally context with a return */
  VM_CONTEXT_TRY,            /**< try context */
  VM_CONTEXT_CATCH,          /**< catch context */
} vm_stack_context_type_t;

ecma_value_t *vm_stack_context_abort(vm_frame_ctx_t *frame_ctx_p,
                                     ecma_value_t *vm_context_top_p);
bool vm_stack_find_finally(vm_frame_ctx_t *frame_ctx_p,
                           ecma_value_t **vm_context_top_ref_p,
                           vm_stack_context_type_t finally_type,
                           uint32_t search_limit);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !VM_H */
