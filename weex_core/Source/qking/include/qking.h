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

#ifndef QKING_H
#define QKING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void *qking_executor_t;

typedef void *qking_external_context_t;

/**
 * Qking API Error object types.
 */
typedef enum {
  QKING_ERROR_NONE = 0,  /**< No Error */
  QKING_ERROR_COMMON,    /**< Error */
  QKING_ERROR_RANGE,     /**< RangeError */
  QKING_ERROR_REFERENCE, /**< ReferenceError */
  QKING_ERROR_SYNTAX,    /**< SyntaxError */
  QKING_ERROR_TYPE,      /**< TypeError */
  QKING_ERROR_URI        /**< URIError */

} qking_error_t;

/**
 * Character type of Qking.
 */
typedef uint8_t qking_char_t;

/**
 * Size type of Qking.
 */
typedef uint32_t qking_size_t;

/**
 * Length type of Qking.
 */
typedef uint32_t qking_length_t;

/**
 * Description of a Qking value.
 */
typedef uint32_t qking_value_t;

/**
 * Native free callback of an object.
 */
typedef void (*qking_object_native_free_callback_t)(void *native_p);

/**
 * Function type applied for each data property of an object.
 */
typedef bool (*qking_object_property_foreach_t)(
    const qking_value_t property_name, const qking_value_t property_value,
    void *user_data_p);

/**
 * Type information of a native pointer.
 */
typedef struct {
  qking_object_native_free_callback_t
      free_cb; /**< the free callback of the native pointer */
} qking_object_native_info_t;

/**
 * GC operational modes.
 */
typedef enum {
  QKING_GC_SEVERITY_LOW, /**< free unused objects, but keep memory
                          *   allocated for performance improvements
                          *   such as property hash tables for large objects */
  QKING_GC_SEVERITY_HIGH /**< free as much memory as possible */
} qking_gc_mode_t;

/**
 * Type of an external function handler.
 */
typedef qking_value_t (*qking_external_handler_t)(
    const qking_value_t function_obj, const qking_value_t this_val,
    const qking_value_t args_p[], const qking_length_t args_count);

/**
 * General executor functions.
 */

qking_executor_t qking_create_executor(qking_external_context_t context);

void qking_set_current_executor(qking_executor_t executor);

qking_executor_t qking_get_current_executor(void);

void qking_destroy_executor(qking_executor_t executor);

qking_external_context_t qking_get_external_context_from_executor(
    qking_executor_t executor);

qking_external_context_t qking_get_current_external_context(void);

bool qking_set_compile_code(qking_executor_t executor, const char *pstr,
                            qking_value_t *error_value);

/**
 * Call function specified by a function value
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no
 * longer needed. error flag must not be set for any arguments of this function.
 *
 * @return returned qking value of the called function
 */
qking_value_t qking_call_function(
    const qking_value_t func_obj_val, /**< function object to call */
    const qking_value_t this_val,     /**< object for 'this' binding */
    const qking_value_t args_p[],     /**< function's call arguments */
    qking_size_t args_count);         /**< number of the arguments */

/**
 * error_value must be checked and released after this call;
 */
bool qking_set_assembly_code(qking_executor_t executor, uint8_t *code,
                             size_t size, qking_value_t *error_value);

/**
 * Register a function in the global object.
 *
 * Note:
 *      returned value must be freed with qking_release_value, when it is no
 * longer needed.
 *
 * @return true value - if the operation was successful,
 *         false - otherwise.
 */
bool qking_external_handler_register_global(
    const char *name_p,                  /**< name of the function */
    qking_external_handler_t handler_p); /**< function callback */

bool qiking_external_variable_register_global(const char *name_p,
                                              qking_value_t var);

/**
 * error_value must be checked and released after this call;
 */
bool qking_execute_code(qking_executor_t executor, qking_value_t *error_value);

/**
 * Create functions of 'qking_value_t'.
 */
qking_value_t qking_create_boolean(bool value);
qking_value_t qking_create_number(double value);
qking_value_t qking_create_number_infinity(bool sign);
qking_value_t qking_create_number_nan(void);
qking_value_t qking_create_undefined(void);
qking_value_t qking_create_null(void);
qking_value_t qking_create_array(uint32_t size);
qking_value_t qking_create_string(const qking_char_t *str_p);
qking_value_t qking_create_object_native_pointer(
    void *native_pointer_p, const qking_object_native_info_t *native_info_p);
qking_value_t qking_create_error(
    qking_error_t error_type, /**< type of error */
    const char *message_p);   /**< value of 'message' property
                               *   of constructed error object */
qking_value_t qking_create_external_function(
    qking_external_handler_t handler_p);
qking_value_t qking_create_number_infinity(bool sign);
qking_value_t qking_create_number_nan(void);
qking_value_t qking_create_object(void);

/**
 * Acquire types with reference counter (increase the references).
 */
qking_value_t qking_acquire_value(qking_value_t value);

/**
 * Release specified Qking API value
 */
void qking_release_value(qking_value_t value); /**< API value */

/**
 * Checker functions of 'qking_value_t'.
 */

bool qking_value_is_string(const qking_value_t value); /**< api value */
bool qking_value_is_number(const qking_value_t value); /**< api value */
bool qking_value_is_boolean(const qking_value_t value);
bool qking_value_is_null(const qking_value_t value);
bool qking_value_is_undefined(const qking_value_t value);
bool qking_value_is_array(const qking_value_t value);
bool qking_value_is_object(const qking_value_t value);
bool qking_value_is_error(const qking_value_t value);
bool qking_value_is_function(const qking_value_t value);

bool qking_value_equal(const qking_value_t left, const qking_value_t right);
bool qking_value_strict_equal(const qking_value_t left,
                              const qking_value_t right);

bool qking_foreach_object_property(const qking_value_t obj_val,
                                   qking_object_property_foreach_t foreach_p,
                                   void *user_data_p);
/**
 * Qking API value type information.
 */
typedef enum {
  QKING_TYPE_NONE = 0u, /**< no type information */
  QKING_TYPE_UNDEFINED, /**< undefined type */
  QKING_TYPE_NULL,      /**< null type */
  QKING_TYPE_BOOLEAN,   /**< boolean type */
  QKING_TYPE_NUMBER,    /**< number type */
  QKING_TYPE_STRING,    /**< string type */
  QKING_TYPE_OBJECT,    /**< object type */
  QKING_TYPE_FUNCTION,  /**< function type */
  QKING_TYPE_ERROR,     /**< error/abort type */
} qking_type_t;

qking_type_t qking_value_get_type(const qking_value_t value);

qking_value_t qking_get_value_from_error(qking_value_t value, bool release);

/**
 * Getter functions of 'qking_value_t'.
 */
bool qking_get_boolean_value(const qking_value_t value);
double qking_get_number_value(const qking_value_t value);
qking_size_t qking_string_to_char_buffer(
    const qking_value_t value, /**< input string value */
    qking_char_t *buffer_p,    /**< [out] output characters buffer */
    qking_size_t buffer_size); /**< size of output buffer */
qking_length_t qking_get_string_length(
    const qking_value_t value); /**< input string */

bool qking_get_object_native_pointer(
    const qking_value_t obj_val, /**< object to get native pointer from */
    void **out_native_pointer_p, /**< [out] native pointer */
    const qking_object_native_info_t *
        *out_native_info_p); /**< [out] the type info of the native pointer */
qking_value_t qking_get_object_keys(const qking_value_t obj_val);

/**
 * Functions for array object values.
 */
qking_size_t qking_get_string_size(const qking_value_t value);
uint32_t qking_get_array_length(const qking_value_t value);

/**
 * General API functions of JS objects.
 */
qking_value_t qking_has_property(const qking_value_t obj_val,
                                 const qking_value_t prop_name_val);
qking_value_t qking_has_own_property(const qking_value_t obj_val,
                                     const qking_value_t prop_name_val);
bool qking_delete_property(const qking_value_t obj_val,
                           const qking_value_t prop_name_val);
bool qking_delete_property_by_index(const qking_value_t obj_val,
                                    uint32_t index);

qking_value_t qking_set_property(const qking_value_t obj_val,
                                 const qking_value_t prop_name_val,
                                 const qking_value_t value_to_set);

qking_value_t qking_get_property(const qking_value_t obj_val,
                                 const qking_value_t prop_name_val);
qking_value_t qking_get_property_by_name(const qking_value_t obj_val,
                                         const char *name_p);
qking_value_t qking_get_property_by_index(const qking_value_t obj_val,
                                          uint32_t index);
qking_value_t qking_set_property_by_index(const qking_value_t obj_val,
                                          uint32_t index,
                                          const qking_value_t value_to_set);

/**
 * Converters of 'qking_value_t'.
 */
qking_value_t qking_json_stringify(const qking_value_t object_to_stringify);
qking_value_t qking_value_to_string(const qking_value_t value);
qking_value_t qking_json_parse_to_object(const qking_value_t value);
qking_value_t qking_json_parse(const qking_char_t *string_p,
                               qking_size_t string_size);

qking_value_t qking_value_debug_print(const qking_value_t value);

qking_value_t qking_print_log(const char *prefix,
                              const qking_value_t func_obj_val,
                              const qking_value_t this_p,
                              const qking_value_t *args_p,
                              const qking_length_t args_cnt);

void qking_gc(qking_gc_mode_t mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !QKING_H */
