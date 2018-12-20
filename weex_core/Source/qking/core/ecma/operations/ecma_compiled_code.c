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

#include "core/include/qking_core.h"
#include "include/qking.h"
#include "ecma_array_object.h"
#include "ecma_compiled_code.h"

ecma_compiled_code_t *ecma_create_compiled_code() {
    size_t size = sizeof(ecma_compiled_code_t);
    size = QKING_ALIGNUP (size, JMEM_ALIGNMENT);
    ecma_compiled_code_t *compiled_code_p = (ecma_compiled_code_t *) jmem_heap_alloc_block(size);
    if (!compiled_code_p) {
        qking_fatal (ERR_OUT_OF_MEMORY);
    }
    compiled_code_p->size = (uint16_t) (size >> JMEM_ALIGNMENT_LOG);
    compiled_code_p->refs = 1;
    compiled_code_p->status_flags = ECMA_CODE_FLAGS_FUNCTION;
    return compiled_code_p;
}

ecma_compiled_function_state_t *ecma_create_compiled_function_state() {
    size_t size = sizeof(ecma_compiled_function_state_t);
    size = QKING_ALIGNUP (size, JMEM_ALIGNMENT);
    ecma_compiled_function_state_t *func_state_p = (ecma_compiled_function_state_t *) jmem_heap_alloc_block(size);
    if (!func_state_p) {
        qking_fatal (ERR_OUT_OF_MEMORY);
    }
    ecma_compiled_code_t *compiled_code_p = &func_state_p->header;
    compiled_code_p->size = (uint16_t) (size >> JMEM_ALIGNMENT_LOG);
    compiled_code_p->refs = 1;
    compiled_code_p->status_flags = ECMA_CODE_FLAGS_FUNCTION;
    func_state_p->constants = ecma_op_create_array_object(NULL, 0, false);
#ifdef QKING_ENABLE_GC_DEBUG
    printf("[gc][info][constants][%i]=>[init]\n", func_state_p->constants);
#endif
    func_state_p->pp_childrens = NULL;
    func_state_p->childrens = 0;
    func_state_p->pp_in_closure = NULL;
    func_state_p->in_closure = 0;
    func_state_p->pp_out_closure = NULL;
    func_state_p->out_closure = 0;
    func_state_p->pc = NULL;
    func_state_p->pcc = 0;
    func_state_p->argc = 0;
    func_state_p->stack_size = 0;
    return func_state_p;
}

void ecma_compiled_func_state_create_instructions(ecma_compiled_function_state_t *function_state, size_t size) {
    size_t alloc_size = size;
    if (alloc_size == 0){
      alloc_size += 1;
    }
    size_t instructions_byte_size = sizeof(unsigned long) * alloc_size;
    function_state->pc = (unsigned long *)jmem_heap_alloc_block(instructions_byte_size);
    function_state->pcc = (uint32_t)(size);
}

void ecma_compiled_func_state_create_constants(ecma_compiled_function_state_t *function_state, size_t size) {
    qking_value_t const_array = qking_create_array((uint32_t)size);
#ifdef QKING_ENABLE_GC_DEBUG
    printf("[gc][info][constants][%i]=>[init]\n", const_array);
#endif
    function_state->constants = const_array;
}

void ecma_compiled_func_state_create_in_closure(ecma_compiled_function_state_t *function_state, size_t size) {
    if (size > 0) {
        function_state->pp_in_closure = (ecma_func_in_closure_t **)jmem_heap_alloc_block(
        sizeof(ecma_func_in_closure_t *) * size);
    }
    else {
        function_state->pp_in_closure = NULL;
    }
    function_state->in_closure = (uint32_t)(size);
}

void ecma_compiled_func_state_create_out_closure(ecma_compiled_function_state_t *function_state, size_t size) {
    if (size > 0) {
        function_state->pp_out_closure = (ecma_func_out_closure_t **)jmem_heap_alloc_block(sizeof(ecma_func_out_closure_t *) * size);
    }
    else {
        function_state->pp_out_closure = NULL;
    }
    function_state->out_closure = (uint32_t)(size);
}

void ecma_compiled_func_state_create_children_array(ecma_compiled_function_state_t *function_state, size_t size) {
  size_t alloc_size = size;
  if (alloc_size == 0){
    alloc_size+=1;
  }
  function_state->pp_childrens =
        (ecma_compiled_function_state_t **)jmem_heap_alloc_block(
            sizeof(ecma_compiled_function_state_t *) * alloc_size);
    function_state->childrens = (uint32_t)(size);
}
