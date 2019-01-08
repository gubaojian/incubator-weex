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
#include "core/ecma/operations/ecma_lex_env.h"
#include "core/ecma/base/ecma_helpers.h"
#include "handler/handler.h"
#include "core/include/qking_core.h"
#include "base/common_logger.h"
#include "core/vm/vm_exec_state.h"
#include "core/jmem/jmem.h"
#include "core/jrt/jrt_libc_includes.h"
#include "port/default/include/qking_port_default.h"
#include "core/vm/qking_vm.h"
#include "core/vm/vm_exec_state.h"
#include "core/vm/vm_ecma_value_helper.h"
#include "core/ecma/operations/ecma_function_object.h"
#ifndef QKING_NDEBUG
#include "ecma_compiled_code.h"
#endif

static void *qking_context_alloc(size_t size, void *cb_data_p) {
  (void)cb_data_p; /* unused */
  return malloc(size);
}

#ifndef QKING_NDEBUG

static void register_js_function(
    const char *name_p,                 /**< name of the function */
    qking_external_handler_t handler_p) /**< function callback */
{
  qking_value_t result_val =
      qkingx_handler_register_global((const qking_char_t *)name_p, handler_p);

  if (qking_value_is_error(result_val)) {
    LOGD("Warning: failed to register '%s' method.", name_p);
  }

  qking_release_value(result_val);
}

#endif

static void qking_context_env_init() {
  qking_init(QKING_INIT_EMPTY);
#ifndef QKING_NDEBUG
  register_js_function("__print__", qkingx_handler_print);
  register_js_function("print", qkingx_handler_print);
  register_js_function("assert", qkingx_handler_assert);
  register_js_function("__build_in_create_err", qkingx_handler_create_err);
#endif
}

qking_vm_exec_state_t *qking_create_vm_exec_state(qking_external_context_t external_context) {
    qking_vm_exec_state_t *exec_state_p = NULL;
    do {
        exec_state_p = (qking_vm_exec_state_t *)(malloc(sizeof(qking_vm_exec_state_t)));
        if (!exec_state_p) {
            break;
        }
        //init
        qking_context_t *context_p = qking_create_context(512 * 1024, qking_context_alloc, NULL);
        if (!context_p) {
            break;
        }
        qking_port_default_set_current_context(context_p);
        qking_context_env_init();
        context_p->external_context_p = external_context;
        context_p->executor_p = exec_state_p;
        exec_state_p->context_p = context_p;
        exec_state_p->external_context_p = external_context;
        exec_state_p->compiled_func_state = NULL;
        
    } while (0);
    
    return exec_state_p;
}

void qking_free_vm_exec_state(qking_vm_exec_state_t *exec_state_p) {
#ifndef QKING_NDEBUG
  QKING_CONTEXT(func_state_p) = exec_state_p->compiled_func_state;
  qking_cleanup();
#endif
  free(exec_state_p->context_p);
  free(exec_state_p);
  qking_port_default_set_current_context(NULL);
}

void qking_vm_exec_state_make_current(qking_vm_exec_state_t *exec_state_p) {
  if (qking_port_get_current_context() == exec_state_p->context_p) {
    qking_port_default_set_current_context(NULL);
  }
  qking_port_default_set_current_context(exec_state_p->context_p);
}

bool qking_vm_exec_state_execute(qking_vm_exec_state_t *exec_state_p, ecma_value_t *err_value) {
  ecma_value_t this_binding = ecma_make_object_value(ecma_builtin_get(ECMA_BUILTIN_ID_GLOBAL));
  ecma_object_t *lex_env_p = ecma_get_global_environment();
  const ecma_compiled_code_t *chunk = (ecma_compiled_code_t *)exec_state_p->compiled_func_state;
  ecma_value_t this_function = ECMA_VALUE_UNDEFINED;
  if (((ecma_compiled_function_state_t *)chunk)->out_closure > 0) {
      ecma_object_t *func_obj_p = ecma_op_create_function_object(lex_env_p, chunk);
      this_function = ecma_make_object_value(func_obj_p);
  }
  ecma_value_t ret_value = vm_run(chunk, this_function, this_binding, lex_env_p, NULL, 0);
  if (!ecma_is_value_undefined(this_function)) {
      ecma_fast_free_value(this_function);
  }
  if (ECMA_IS_VALUE_ERROR(ret_value)) {
    ret_value = ecma_create_error_reference_from_context();
  }
  bool is_err = ecma_is_value_error_reference(ret_value);
  if (is_err) {
    qking_value_t err_ret_value = qking_get_value_from_error(ret_value, true);
    QKING_TO_LOG_STR_START(err_str, err_ret_value);
    LOGE("Unhandled Expception %s\n", err_str);
    if (err_value) {
      *err_value = qking_create_string_sz((const qking_char_t *)err_str, (qking_size_t) strlen(err_str));
    }
    QKING_TO_LOG_STR_FINISH(err_str);
    qking_release_value(err_ret_value);
  } else {
    ecma_free_value(QKING_CONTEXT (error_value));
    QKING_CONTEXT (error_value) = ECMA_VALUE_UNDEFINED;
    if (err_value) {
      *err_value = ECMA_VALUE_UNDEFINED;
    }
  }

  ecma_free_value(this_binding);
  return !is_err;
}


