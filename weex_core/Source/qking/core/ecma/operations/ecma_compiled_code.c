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
#include "core/include/qking_core.h"
#include "include/qking.h"
#include "ecma_array_object.h"
#include "ecma_compiled_code.h"
#ifndef QKING_NDEBUG
#include "core/ecma/base/ecma_helpers.h"
#endif

ecma_compiled_code_t *ecma_create_compiled_code() {
    size_t size = sizeof(ecma_compiled_code_t);
    size = QKING_ALIGNUP (size, JMEM_ALIGNMENT);
    ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block(size);
    if (!bytecode_p) {
        qking_fatal (ERR_OUT_OF_MEMORY);
    }
    bytecode_p->size = (uint16_t) (size >> JMEM_ALIGNMENT_LOG);
    bytecode_p->refs = ECMA_BYTECODE_REF_ONE;
    bytecode_p->status_flags = ECMA_CODE_FLAGS_FUNCTION;
#ifdef QKING_ENABLE_GC_DEBUG
    jmem_cpointer_t cp_pointer = ECMA_NULL_POINTER;
    ECMA_SET_NON_NULL_POINTER(cp_pointer, bytecode_p);
    printf("[gc][%i]=>[new][%d]\n", cp_pointer, bytecode_p->refs);
#endif
#ifdef JMEM_STATS
    jmem_stats_allocate_byte_code_bytes (((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
#endif /* JMEM_STATS */
    return bytecode_p;
}

ecma_compiled_function_state_t *ecma_create_compiled_function_state() {
    size_t size = sizeof(ecma_compiled_function_state_t);
    size = QKING_ALIGNUP (size, JMEM_ALIGNMENT);
    ecma_compiled_function_state_t *func_state_p = (ecma_compiled_function_state_t *) jmem_heap_alloc_block(size);
    if (!func_state_p) {
        qking_fatal (ERR_OUT_OF_MEMORY);
    }
    ecma_compiled_code_t *bytecode_p = &func_state_p->header;
    bytecode_p->size = (uint16_t) (size >> JMEM_ALIGNMENT_LOG);
    bytecode_p->refs = ECMA_BYTECODE_REF_ONE;
    bytecode_p->status_flags = ECMA_CODE_FLAGS_FUNCTION;
    func_state_p->constants = ECMA_VALUE_UNDEFINED;
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
#ifdef QKING_ENABLE_GC_DEBUG
    jmem_cpointer_t cp_pointer = ECMA_NULL_POINTER;
    ECMA_SET_NON_NULL_POINTER(cp_pointer, bytecode_p);
    printf("[gc][%i]=>[new][%d]\n", cp_pointer, bytecode_p->refs);
#endif
#ifdef JMEM_STATS
    jmem_stats_allocate_byte_code_bytes (((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
#endif /* JMEM_STATS */
    return func_state_p;
}

void ecma_compiled_func_state_create_instructions(ecma_compiled_function_state_t *function_state, size_t size) {
    if (size > 0) {
        size_t instructions_byte_size = sizeof(unsigned long) * size;
        function_state->pc = (unsigned long *)jmem_heap_alloc_block(instructions_byte_size);
    }
    else {
        function_state->pc = NULL;
    }
    function_state->pcc = (uint32_t)(size);
}

void ecma_compiled_func_state_create_constants(ecma_compiled_function_state_t *function_state_p, size_t size) {
    QKING_ASSERT(qking_value_is_undefined(function_state_p->constants));
    function_state_p->constants = qking_create_array((uint32_t)size);
#ifdef QKING_ENABLE_GC_DEBUG
    printf("[gc][info][constants][%i]=>[init]\n", function_state_p->constants);
#endif
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
    if (size > 0) {
        function_state->pp_childrens =
        (ecma_compiled_function_state_t **)jmem_heap_alloc_block(sizeof(ecma_compiled_function_state_t *) * size);
        function_state->childrens = (uint32_t)(size);
    }
    else {
        function_state->pp_childrens = NULL;
    }
}

#ifndef QKING_NDEBUG

void ecma_finalize_compiled_function_state(ecma_compiled_function_state_t *function_state_p) {
    if (!function_state_p) {
        return;
    }
    if (function_state_p->pc) {
        QKING_ASSERT(function_state_p->pcc);
        size_t instructions_size = sizeof(unsigned long) * function_state_p->pcc;
        jmem_heap_free_block(function_state_p->pc, instructions_size);
        function_state_p->pc = NULL;
        function_state_p->pcc = 0;
    }
    if (qking_value_is_array(function_state_p->constants)) {
        qking_release_value(function_state_p->constants);
        function_state_p->constants = ECMA_VALUE_UNDEFINED;
    }
    if (function_state_p->pp_in_closure) {
        QKING_ASSERT(function_state_p->in_closure);
        ecma_func_in_closure_t **in_closure_pp = function_state_p->pp_in_closure;
        for (int i = 0; i < function_state_p->in_closure; i++) {
            ecma_func_in_closure_t *in_closure_p = in_closure_pp[i];
            QKING_ASSERT(in_closure_p);
            jmem_heap_free_block(in_closure_p, sizeof(ecma_func_in_closure_t));
        }
        jmem_heap_free_block(in_closure_pp, sizeof(ecma_func_in_closure_t *) * function_state_p->in_closure);
        function_state_p->pp_in_closure = NULL;
    }
    if (function_state_p->pp_out_closure) {
        QKING_ASSERT(function_state_p->out_closure);
        jmem_heap_free_block(function_state_p->pp_out_closure, sizeof(ecma_func_out_closure_t *) * function_state_p->out_closure);
        function_state_p->pp_out_closure = NULL;
        function_state_p->out_closure = 0;
    }
    if (function_state_p->pp_childrens) {
        QKING_ASSERT(function_state_p->childrens);
        ecma_compiled_function_state_t **pp_childrens = function_state_p->pp_childrens;
        for (int i = 0; i < function_state_p->childrens; i++) {
            ecma_compiled_function_state_t *children_p = pp_childrens[i];
            QKING_ASSERT(children_p);
            ecma_finalize_compiled_function_state(children_p);
        }
        jmem_heap_free_block(pp_childrens, sizeof(ecma_compiled_function_state_t *) * function_state_p->childrens);
        function_state_p->pp_childrens = NULL;
        function_state_p->childrens = 0;
    }
    ecma_compiled_code_t *bytecode_p = &function_state_p->header;
    ecma_bytecode_deref (bytecode_p);
}

#endif
