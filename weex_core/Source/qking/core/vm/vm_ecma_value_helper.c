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

#include "core/ecma/operations/ecma_lex_env.h"
#include "core/ecma/base/ecma_helpers.h"
#include "handler/handler.h"
#include "core/include/qking_core.h"
#include "base/common_logger.h"
#include "core/vm/vm_exec_state.h"
#include "core/jmem/jmem.h"
#include "core/jrt/jrt_libc_includes.h"
#include "port/default/include/qking_port_default.h"
#include "core/vm/vm.h"
#include "core/vm/vm_ecma_value_helper.h"
#include "vm_ecma_value_helper.h"

const char *qking_convert_to_log_str_from_value(ecma_value_t value) {
  const char *prefix = "";
  const char *result = NULL;
  ecma_value_t to_print = ecma_fast_copy_value(value);
  if (ECMA_IS_VALUE_ERROR(to_print)) {
    // direct type don't need free;
    to_print = ecma_create_error_reference_from_context();
    prefix = "<ERR>";
  }

  if (ecma_is_value_error_reference(value)) {
    ecma_fast_free_value(to_print);
    to_print = qking_get_value_from_error(to_print, true);

    if (strlen(prefix) == 0) {
      prefix = "<ERR-Ref>";
    }
  }

  qking_value_t str_val = qking_value_to_string(to_print);
  if (qking_value_is_error(str_val)) {
    const char *str_content = "[can't convert to string value]";
    size_t string_size = sizeof(str_content);
    size_t prefix_size = strlen(prefix);
    size_t final_buffer_size = prefix_size + string_size + 1;
    char *buffer = jmem_system_malloc(sizeof(char) * final_buffer_size);
    strcpy(buffer, prefix);
    strcpy(buffer + prefix_size, str_content);
    buffer[final_buffer_size - 1] = '\0';
    result = buffer;
  } else {
    QKING_ASSERT(ecma_is_value_string(str_val));
    ecma_string_t *str_p = ecma_get_string_from_value(str_val);
    lit_utf8_size_t string_size = ecma_string_get_size(str_p);

    size_t prefix_size = strlen(prefix);
    size_t final_buffer_size = prefix_size + string_size + 1;
    lit_utf8_byte_t *buffer = jmem_system_malloc(sizeof(lit_utf8_byte_t) * final_buffer_size);

    lit_utf8_size_t result_size =
        ecma_string_copy_to_cesu8_buffer(str_p, buffer + prefix_size, string_size);
    buffer[final_buffer_size - 1] = '\0';
    memcpy(buffer, prefix, prefix_size * sizeof(char));
    QKING_ASSERT(result_size == string_size);

    result = (const char *) buffer;
  }

  qking_release_value(str_val);
  ecma_fast_free_value(to_print);
  return result;
}
const char *qking_get_log_str_from_ecma_string(ecma_string_t *str_p) {
  const char * result = NULL;
  if(str_p == NULL){
    char * s = "NULL_PTR_ECMA_STRING";
    size_t final_buffer_size = strlen(s) + 1;
    lit_utf8_byte_t *buffer = jmem_system_malloc(sizeof(lit_utf8_byte_t) * final_buffer_size);
    strcpy((char*)buffer,s);
    result = (const char *) buffer;
  } else {
    lit_utf8_size_t string_size = ecma_string_get_size(str_p);

    size_t final_buffer_size = string_size + 1;
    lit_utf8_byte_t *buffer = jmem_system_malloc(sizeof(lit_utf8_byte_t) * final_buffer_size);

    lit_utf8_size_t result_size =
        ecma_string_copy_to_cesu8_buffer(str_p, buffer, string_size);
    buffer[final_buffer_size - 1] = '\0';
    QKING_ASSERT(result_size == string_size);

    result = (const char *) buffer;
  }
  return result;
}
