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
#ifndef QKING_ROOT_VM_EXEC_STATE_H
#define QKING_ROOT_VM_EXEC_STATE_H

#include "core/ecma/base/ecma_globals.h"
#include "core/jcontext/jcontext.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  ecma_compiled_function_state_t *compiled_func_state;
  qking_context_t *context_p;
  qking_external_context_t *external_context_p;

} qking_vm_exec_state_t;

qking_vm_exec_state_t *qking_create_vm_exec_state(
    qking_external_context_t external_context);

void qking_free_vm_exec_state(qking_vm_exec_state_t *exec_state_p);

void qking_vm_exec_state_make_current(qking_vm_exec_state_t *exec_state_p);

bool qking_vm_exec_state_execute(qking_vm_exec_state_t *exec_state_p,
                                 ecma_value_t *err_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // QKING_ROOT_VM_EXEC_STATE_H
