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
#ifndef VM_DEFINES_H
#define VM_DEFINES_H

#include "ecma_globals.h"

/**
 * Context of interpreter, related to a JS stack frame
 */
typedef struct vm_frame_ctx_t {
  const ecma_compiled_code_t
      *bytecode_header_p;       /**< currently executed byte-code data */
  ecma_register_t *registers_p; /**< register start pointer */
  ecma_value_t *exec_context_p; /**< context start pointer */
  ecma_object_t *lex_env_p;     /**< current lexical environment */
  struct vm_frame_ctx_t *prev_context_p; /**< previous context */
  ecma_value_t this_binding;             /**< this binding */
  ecma_value_t call_block_result; /**< preserve block result during a call */
#ifdef QKING_ENABLE_LINE_INFO
  ecma_value_t
      resource_name;      /**< current resource name (usually a file name) */
  uint32_t current_line;  /**< currently executed line */
#endif                    /* QKING_ENABLE_LINE_INFO */
  uint16_t context_depth; /**< current context depth */
  uint8_t call_operation; /**< perform a call or construct operation */
  ecma_value_t this_function; /**< current function object */

  unsigned long *pc_current_p;
  unsigned long *pc_start_p; /**< current function object */
  uint32_t pc_count;         /**< current function object */

} vm_frame_ctx_t;

/**
 * Helper for += on uint16_t values.
 */
#define VM_PLUS_EQUAL_U16(base, value) (base) = (uint16_t)((base) + (value))

/**
 * Helper for -= on uint16_t values.
 */
#define VM_MINUS_EQUAL_U16(base, value) (base) = (uint16_t)((base) - (value))

/**
 * @}
 * @}
 */

#endif /* !VM_DEFINES_H */
