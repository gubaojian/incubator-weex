#include "stdlib.h"
#include "handler/handler.h"
#include "qking/include/qking.h"
#include "base/common_logger.h"
#include "core/include/qking_core.h"
#include "core/jcontext/jcontext.h"
#include "core/lit/lit_strings.h"
#include "decoder/exec_state_decode.h"
#include "core/vm/vm_exec_state.h"
#include "ecma_helpers.h"
#include "stdbool.h"
#ifdef QKING_ENABLE_GC_DEBUG
#include "core/ecma/base/ecma_gc.h"
#endif

qking_executor_t qking_create_executor(qking_external_context_t context) {
  qking_executor_t executor = qking_create_vm_exec_state(context);
  if (executor) {
    qking_set_current_executor(executor);
  }
  return executor;
}

void qking_set_current_executor(qking_executor_t executor) {
  qking_vm_exec_state_make_current((qking_vm_exec_state_t *)executor);
}

void qking_destroy_executor(qking_executor_t executor) {
  qking_set_current_executor(executor);
  qking_free_vm_exec_state((qking_vm_exec_state_t *)executor);
}

bool qking_set_assembly_code(qking_executor_t executor, uint8_t *code,
                             size_t size, qking_value_t *error_value) {
  exec_state_decoder_t *ptr = qking_create_decoder((qking_vm_exec_state_t *)executor, code, size);
  if (!ptr){
    return false;
  }
  const char * err_msg = NULL;
  bool succ = qking_decoding_binary(ptr, &err_msg);
  if (!succ) {
    QKING_ASSERT(err_msg);
    *error_value = qking_create_string((const qking_char_t *) err_msg);
  }
  qking_free_decoder(ptr);
  return succ;
}

#ifdef CONFIG_DISABLE_COMPILER_BUILTIN
bool qking_set_compile_code(qking_executor_t executor, const char *pstr,
                            qking_value_t *error_value){
  return false;
}
#endif

bool qking_execute_code(qking_executor_t executor, qking_value_t *error_value) {
  return qking_vm_exec_state_execute((qking_vm_exec_state_t *)executor, error_value);
}

bool qking_external_handler_register_global(
    const char *name_p, qking_external_handler_t handler_p) {
  bool finished = true;
  qking_value_t result =
      qkingx_handler_register_global((const qking_char_t *)name_p, handler_p);
  if (qking_value_is_error(result)) {
    LOGD("Warning: failed to register '%s' method.", name_p);
    finished = false;
  }
  qking_release_value(result);
  return finished;
}

bool qiking_external_variable_register_global(const char *name_p,
                                              qking_value_t var) {
  bool ret = true;
  qking_value_t result = qkingx_variable_register_global((const qking_char_t *)name_p, var);
  if(qking_value_is_error(result)){
    ret = false;
  }
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_info_ignore(var);
#endif
  qking_release_value(result);
  return ret;
}

qking_external_context_t qking_get_current_external_context(void) {
  return qking_port_get_current_context()->external_context_p;
}

qking_executor_t qking_get_current_executor(void) {
  return qking_port_get_current_context()->executor_p;
}

qking_external_context_t qking_get_external_context_from_executor(qking_executor_t executor) {
  return ((qking_vm_exec_state_t*)executor)->external_context_p;
}

qking_value_t qking_create_object_native_pointer(
    void *native_pointer_p, const qking_object_native_info_t *native_info_p) {
  qking_value_t object = qking_create_object();
  qking_set_object_native_pointer(object, native_pointer_p, native_info_p);
  return object;
}

qking_value_t qking_json_parse_to_object(const qking_value_t value) {
  qking_value_t result = qking_create_undefined();
  do {
    if (!qking_value_is_string(value)) {
      break;
    }
    ecma_string_t *str_p = ecma_get_string_from_value(value);
    uint32_t string_size = ecma_string_get_length(str_p);
    qking_char_t *buffer = (qking_char_t *)jmem_system_malloc(sizeof(qking_char_t)*(string_size + 1));
    if (!buffer) {
      break;
    }
    qking_string_to_char_buffer(value, buffer, string_size + 1);
    result = qking_json_parse(buffer, string_size);
    jmem_system_free(buffer);
  } while (0);

  return result;
}

qking_value_t qking_get_property_by_name(const qking_value_t obj_val,
                                         const char *name_p) {
  qking_value_t name_prop_var =
      qking_create_string((const qking_char_t *)name_p);
  qking_value_t result = qking_get_property(obj_val, name_prop_var);
  qking_release_value(name_prop_var);
  return result;
}

qking_value_t qking_value_debug_print(const qking_value_t value) {
  if (qking_value_is_object(value)) {
    return qking_json_stringify(value);
  } else {
    return qking_value_to_string(value);
  }
}
