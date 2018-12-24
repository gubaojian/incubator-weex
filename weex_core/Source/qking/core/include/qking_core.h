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

#ifndef QKING_CORE_H
#define QKING_CORE_H

#include "qking/include/qking.h"
#include "qking_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \addtogroup qking Qking engine interface
 * @{
 */

/**
 * Major version of Qking API.
 */
#define QKING_API_MAJOR_VERSION 1

/**
 * Minor version of Qking API.
 */
#define QKING_API_MINOR_VERSION 0

/**
 * Qking init flags.
 */
typedef enum {
  QKING_INIT_EMPTY = (0u),             /**< empty flag set */
  QKING_INIT_SHOW_OPCODES = (1u << 0), /**< dump byte-code to log after parse */
  QKING_INIT_SHOW_REGEXP_OPCODES =
      (1u << 1), /**< dump regexp byte-code to log after compilation */
  QKING_INIT_MEM_STATS = (1u << 2), /**< dump memory statistics */
  QKING_INIT_MEM_STATS_SEPARATE =
      (1u << 3),                   /**< deprecated, an unused placeholder now */
  QKING_INIT_DEBUGGER = (1u << 4), /**< deprecated, an unused placeholder now */
} qking_init_flag_t;

/**
 * Qking feature types.
 */
typedef enum {
  QKING_FEATURE_CPOINTER_32_BIT, /**< 32 bit compressed pointers */
  QKING_FEATURE_ERROR_MESSAGES,  /**< error messages */
  QKING_FEATURE_JS_PARSER,       /**< js-parser */
  QKING_FEATURE_MEM_STATS,       /**< memory statistics */
  QKING_FEATURE_PARSER_DUMP,     /**< parser byte-code dumps */
  QKING_FEATURE_REGEXP_DUMP,     /**< regexp byte-code dumps */
  QKING_FEATURE_SNAPSHOT_SAVE,   /**< saving snapshot files */
  QKING_FEATURE_SNAPSHOT_EXEC,   /**< executing snapshot files */
  QKING_FEATURE_DEBUGGER,        /**< debugging */
  QKING_FEATURE_VM_EXEC_STOP,    /**< stopping ECMAScript execution */
  QKING_FEATURE_JSON,            /**< JSON support */
  QKING_FEATURE_PROMISE,         /**< promise support */
  QKING_FEATURE_TYPEDARRAY,      /**< Typedarray support */
  QKING_FEATURE_DATE,            /**< Date support */
  QKING_FEATURE_REGEXP,          /**< Regexp support */
  QKING_FEATURE_LINE_INFO,       /**< line info available */
  QKING_FEATURE_LOGGING,         /**< logging */
  QKING_FEATURE__COUNT /**< number of features. NOTE: must be at the end of the
                          list */
} qking_feature_t;

/**
 * Description of ECMA property descriptor.
 */
typedef struct {
  /** Is [[Value]] defined? */
  bool is_value_defined;

  /** Is [[Get]] defined? */
  bool is_get_defined;

  /** Is [[Set]] defined? */
  bool is_set_defined;

  /** Is [[Writable]] defined? */
  bool is_writable_defined;

  /** [[Writable]] */
  bool is_writable;

  /** Is [[Enumerable]] defined? */
  bool is_enumerable_defined;

  /** [[Enumerable]] */
  bool is_enumerable;

  /** Is [[Configurable]] defined? */
  bool is_configurable_defined;

  /** [[Configurable]] */
  bool is_configurable;

  /** [[Value]] */
  qking_value_t value;

  /** [[Get]] */
  qking_value_t getter;

  /** [[Set]] */
  qking_value_t setter;
} qking_property_descriptor_t;

/**
 * Description of Qking heap memory stats.
 * It is for memory profiling.
 */
typedef struct {
  size_t version;              /**< the version of the stats struct */
  size_t size;                 /**< heap total size */
  size_t allocated_bytes;      /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */
  size_t reserved[4];          /**< padding for future extensions */
} qking_heap_stats_t;

/**
 * Callback which tells whether the ECMAScript execution should be stopped.
 *
 * As long as the function returns with undefined the execution continues.
 * When a non-undefined value is returned the execution stops and the value
 * is thrown by the engine (if the error flag is not set for the returned
 * value the engine automatically sets it).
 *
 * Note: if the function returns with a non-undefined value it
 *       must return with the same value for future calls.
 */
typedef qking_value_t (*qking_vm_exec_stop_callback_t)(void *user_p);

/**
 * Function type applied for each object in the engine.
 */
typedef bool (*qking_objects_foreach_t)(const qking_value_t object,
                                        void *user_data_p);

/**
 * Function type applied for each matching object in the engine.
 */
typedef bool (*qking_objects_foreach_by_native_info_t)(
    const qking_value_t object, void *object_data_p, void *user_data_p);

/**
 * User context item manager
 */
typedef struct {
  /**
   * Callback responsible for initializing a context item, or NULL to zero out
   * the memory. This is called lazily, the first time qking_get_context_data ()
   * is called with this manager.
   *
   * @param [in] data The buffer that Qking allocated for the manager. The
   * buffer is zeroed out. The size is determined by the bytes_needed field. The
   * buffer is kept alive until qking_cleanup () is called.
   */
  void (*init_cb)(void *data);

  /**
   * Callback responsible for deinitializing a context item, or NULL. This is
   * called as part of qking_cleanup (), right *before* the VM has been cleaned
   * up. This is a good place to release strong references to qking_value_t's
   * that the manager may be holding.
   * Note: because the VM has not been fully cleaned up yet,
   * qking_object_native_info_t free_cb's can still get called *after* all
   * deinit_cb's have been run. See finalize_cb for a callback that is
   * guaranteed to run *after* all free_cb's have been run.
   *
   * @param [in] data The buffer that Qking allocated for the manager.
   */
  void (*deinit_cb)(void *data);

  /**
   * Callback responsible for finalizing a context item, or NULL. This is called
   * as part of qking_cleanup (), right *after* the VM has been cleaned up and
   * destroyed and qking_... APIs cannot be called any more. At this point, all
   * values in the VM have been cleaned up. This is a good place to clean up
   * native state that can only be cleaned up at the very end when there are no
   * more VM values around that may need to access that state.
   *
   * @param [in] data The buffer that Qking allocated for the manager.
   * After returning from this callback, the data pointer may no longer be used.
   */
  void (*finalize_cb)(void *data);

  /**
   * Number of bytes to allocate for this manager. This is the size of the
   * buffer that Qking will allocate on behalf of the manager. The pointer
   * to this buffer is passed into init_cb, deinit_cb and finalize_cb. It is
   * also returned from the qking_get_context_data () API.
   */
  size_t bytes_needed;
} qking_context_data_manager_t;

/**
 * Function type for allocating buffer for Qking context.
 */
typedef void *(*qking_context_alloc_t)(size_t size, void *cb_data_p);

/**
 * An opaque declaration of the Qking context structure.
 */
typedef struct qking_context_t qking_context_t;

void qking_init(qking_init_flag_t flags);
void qking_cleanup(void);
void qking_register_magic_strings(const qking_char_t *const *ex_str_items_p,
                                  uint32_t count,
                                  const qking_length_t *str_lengths_p);

void *qking_get_context_data(const qking_context_data_manager_t *manager_p);

bool qking_get_memory_stats(qking_heap_stats_t *out_stats_p);

/**
 * Get the global context.
 */
qking_value_t qking_get_global_object(void);

/**
 * Checker functions of 'qking_value_t'.
 */
bool qking_value_is_abort(const qking_value_t value);
bool qking_value_is_constructor(const qking_value_t value);
bool qking_value_is_promise(const qking_value_t value);

/**
 * Checker function of whether the specified compile feature is enabled.
 */
bool qking_is_feature_enabled(const qking_feature_t feature);

/**
 * Error manipulation functions.
 */
qking_value_t qking_create_abort_from_value(qking_value_t value, bool release);
qking_value_t qking_create_error_from_value(qking_value_t value, bool release);
qking_value_t qking_get_value_from_error(qking_value_t value, bool release);

/**
 * Error object function(s).
 */
qking_error_t qking_get_error_type(qking_value_t value);

/**
 * Functions for string values.
 */
qking_size_t qking_get_utf8_string_size(const qking_value_t value);
qking_length_t qking_get_utf8_string_length(const qking_value_t value);
qking_size_t qking_string_to_utf8_char_buffer(const qking_value_t value,
                                              qking_char_t *buffer_p,
                                              qking_size_t buffer_size);
qking_size_t qking_substring_to_char_buffer(const qking_value_t value,
                                            qking_length_t start_pos,
                                            qking_length_t end_pos,
                                            qking_char_t *buffer_p,
                                            qking_size_t buffer_size);
qking_size_t qking_substring_to_utf8_char_buffer(const qking_value_t value,
                                                 qking_length_t start_pos,
                                                 qking_length_t end_pos,
                                                 qking_char_t *buffer_p,
                                                 qking_size_t buffer_size);

/**
 * Converters of 'qking_value_t'.
 */
bool qking_value_to_boolean(const qking_value_t value);
qking_value_t qking_value_to_number(const qking_value_t value);
qking_value_t qking_value_to_object(const qking_value_t value);
qking_value_t qking_value_to_primitive(const qking_value_t value);

/**
 * Create functions of API values.
 */
qking_value_t qking_create_error_sz(qking_error_t error_type,
                                    const qking_char_t *message_p,
                                    qking_size_t message_size);
qking_value_t qking_create_promise(void);
qking_value_t qking_create_string_from_utf8(const qking_char_t *str_p);
qking_value_t qking_create_string_sz_from_utf8(const qking_char_t *str_p,
                                               qking_size_t str_size);
qking_value_t qking_create_string_sz(const qking_char_t *str_p,
                                     qking_size_t str_size);

void qking_init_property_descriptor_fields(
    qking_property_descriptor_t *prop_desc_p);
qking_value_t qking_define_own_property(
    const qking_value_t obj_val, const qking_value_t prop_name_val,
    const qking_property_descriptor_t *prop_desc_p);

bool qking_get_own_property_descriptor(
    const qking_value_t obj_val, const qking_value_t prop_name_val,
    qking_property_descriptor_t *prop_desc_p);
void qking_free_property_descriptor_fields(
    const qking_property_descriptor_t *prop_desc_p);

qking_value_t qking_construct_object(const qking_value_t func_obj_val,
                                     const qking_value_t args_p[],
                                     qking_size_t args_count);

qking_value_t qking_get_prototype(const qking_value_t obj_val);
qking_value_t qking_set_prototype(const qking_value_t obj_val,
                                  const qking_value_t proto_obj_val);

void qking_set_object_native_pointer(
    const qking_value_t obj_val, void *native_pointer_p,
    const qking_object_native_info_t *native_info_p);

bool qking_objects_foreach(qking_objects_foreach_t foreach_p, void *user_data);
bool qking_objects_foreach_by_native_info(
    const qking_object_native_info_t *native_info_p,
    qking_objects_foreach_by_native_info_t foreach_p, void *user_data_p);

/**
 * Promise resolve/reject functions.
 */
qking_value_t qking_resolve_or_reject_promise(qking_value_t promise,
                                              qking_value_t argument,
                                              bool is_resolve);

/**
 * Input validator functions.
 */
bool qking_is_valid_utf8_string(const qking_char_t *utf8_buf_p,
                                qking_size_t buf_size);
bool qking_is_valid_cesu8_string(const qking_char_t *cesu8_buf_p,
                                 qking_size_t buf_size);

/*
 * Dynamic memory management functions.
 */
void *qking_heap_alloc(size_t size);
void qking_heap_free(void *mem_p, size_t size);

/*
 * External context functions.
 */
qking_context_t *qking_create_context(uint32_t heap_size,
                                      qking_context_alloc_t alloc,
                                      void *cb_data_p);

/**
 * Miscellaneous functions.
 */
void qking_set_vm_exec_stop_callback(qking_vm_exec_stop_callback_t stop_cb,
                                     void *user_p, uint32_t frequency);

/**
 * Array buffer components.
 */
bool qking_value_is_arraybuffer(const qking_value_t value);
qking_value_t qking_create_arraybuffer(const qking_length_t size);
qking_value_t qking_create_arraybuffer_external(
    const qking_length_t size, uint8_t *buffer_p,
    qking_object_native_free_callback_t free_cb);
qking_length_t qking_arraybuffer_write(const qking_value_t value,
                                       qking_length_t offset,
                                       const uint8_t *buf_p,
                                       qking_length_t buf_size);
qking_length_t qking_arraybuffer_read(const qking_value_t value,
                                      qking_length_t offset, uint8_t *buf_p,
                                      qking_length_t buf_size);
qking_length_t qking_get_arraybuffer_byte_length(const qking_value_t value);
uint8_t *qking_get_arraybuffer_pointer(const qking_value_t value);

/**
 * TypedArray functions.
 */

/**
 * TypedArray types.
 */
typedef enum {
  QKING_TYPEDARRAY_INVALID = 0,
  QKING_TYPEDARRAY_UINT8,
  QKING_TYPEDARRAY_UINT8CLAMPED,
  QKING_TYPEDARRAY_INT8,
  QKING_TYPEDARRAY_UINT16,
  QKING_TYPEDARRAY_INT16,
  QKING_TYPEDARRAY_UINT32,
  QKING_TYPEDARRAY_INT32,
  QKING_TYPEDARRAY_FLOAT32,
  QKING_TYPEDARRAY_FLOAT64,
} qking_typedarray_type_t;

bool qking_value_is_typedarray(qking_value_t value);
qking_value_t qking_create_typedarray(qking_typedarray_type_t type_name,
                                      qking_length_t length);
qking_value_t qking_create_typedarray_for_arraybuffer_sz(
    qking_typedarray_type_t type_name, const qking_value_t arraybuffer,
    qking_length_t byte_offset, qking_length_t length);
qking_value_t qking_create_typedarray_for_arraybuffer(
    qking_typedarray_type_t type_name, const qking_value_t arraybuffer);
qking_typedarray_type_t qking_get_typedarray_type(qking_value_t value);
qking_length_t qking_get_typedarray_length(qking_value_t value);
qking_value_t qking_get_typedarray_buffer(qking_value_t value,
                                          qking_length_t *byte_offset,
                                          qking_length_t *byte_length);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKING_CORE_H */
