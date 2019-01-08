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
//
// Created by Xu Jiacheng on 2018/12/25.
//

#ifndef QKING_ROOT_RAX_CORE_UTIL_H
#define QKING_ROOT_RAX_CORE_UTIL_H

#include <sstream>
#include "rax_common.h"

#define THROW_JS_ERR_IF(expr, msg)                              \
  do {                                                          \
    if (!!(expr)) {                                             \
      qking_release_value(func_ret);                            \
      func_ret = qking_create_error(QKING_ERROR_COMMON, (msg)); \
      throw rax_js_err();                                       \
    }                                                           \
  } while (0)

#define CHECK_TMP_VAR(expr)                  \
  do {                                       \
    qking_value_t _expr_value = (expr);      \
    if (qking_value_is_error(_expr_value)) { \
      qking_release_value(func_ret);         \
      func_ret = _expr_value;                \
      throw rax_js_err();                    \
    }                                        \
  } while (0)

#define CHECK_TMP_VAR_RELEASE(expr, release) \
  do {                                       \
    qking_value_t _expr_value = (expr);      \
    if (qking_value_is_error(_expr_value)) { \
      qking_release_value(func_ret);         \
      qking_release_value((release));        \
      func_ret = _expr_value;                \
      throw rax_js_err();                    \
    }                                        \
  } while (0)

#define CHECK_RESOURCE(expr)                       \
  do {                                             \
    qking_value_t _expr_value = (expr);            \
    if (qking_value_is_error(_expr_value)) {       \
      qking_release_value(func_ret);               \
      func_ret = qking_acquire_value(_expr_value); \
      throw rax_js_err();                          \
    }                                              \
  } while (0)

#define COMMON_START() qking_value_t func_ret = qking_create_undefined();

#define COMMON_RESOURCE() \
  try {                   \
  ((void)0)

#define COMMON_FINALIZE()                                        \
  }                                                              \
  catch (const rax_js_err& e) {                                  \
  }                                                              \
  catch (const rax_common_err& e) {                              \
    qking_release_value(func_ret);                               \
    func_ret = qking_create_error(QKING_ERROR_COMMON, e.what()); \
  }                                                              \
  ((void)0)

#define COMMON_END() return func_ret

#ifdef QKING_RAX_DEBUG
#define RAX_LOGD(...) LOGI("[RAX] " __VA_ARGS__)
#else
#define RAX_LOGD(...) ((void)0)
#endif

#define RAX_LOGI(...) LOGI("[RAX] " __VA_ARGS__)
#define RAX_LOGW(...) LOGW("[RAX] " __VA_ARGS__)
#define RAX_LOGE(...) LOGE("[RAX] " __VA_ARGS__)

RAX_NAME_SPACE_BEGIN

template <typename T>
std::string to_string(T n) {
  std::ostringstream ss;
  ss << n;
  return ss.str();
}

class rax_js_err : public std::exception {};

class rax_common_err : public std::logic_error {
 public:
  rax_common_err(const char* err);
  rax_common_err(const std::string& err);
};

qking_value_t rax_flatten_children(const qking_value_t* array,
                                   qking_length_t size);

qking_value_t rax_flatten_style(qking_value_t value);

bool qking_value_add_all_to(const qking_value_t dest, const qking_value_t src);

std::string string_from_qking_string_value(const qking_value_t var);

std::string string_from_qking_string_value_lit(const qking_magic_str_t name);

std::string string_from_qking_get_property_by_name(const qking_value_t obj_val,
                                                   const char* name_p);

std::string string_from_qking_json_stringify(
    const qking_value_t object_to_stringify);

std::string string_from_qking_to_string(const qking_value_t object_to_string);

std::string string_from_qking_error(const qking_value_t err);

std::string string_from_qking_get_property_by_index(const qking_value_t obj_val,
                                                    uint32_t index);

void make_optional_call_on(qking_value_t class_inst,
                           qking_magic_str_t func_name, const char* process_msg,
                           const qking_value_t* args_p = nullptr,
                           qking_size_t args_count = 0);

qking_value_t make_optional_call_on_ret(qking_value_t class_inst,
                                        qking_magic_str_t func_name,
                                        const char* process_msg,
                                        bool& has_function,
                                        const qking_value_t* args_p = nullptr,
                                        qking_size_t args_count = 0);

qking_value_t get_optional_property(
    qking_value_t obj, qking_magic_str_t prop_name, const char* process_msg,
    const char* obj_name,
    std::function<void(qking_value_t)> succ_callback = nullptr);

bool set_optional_property(qking_value_t obj, qking_magic_str_t prop_name,
                           qking_value_t value, const char* process_msg,
                           const char* obj_msg);

class QKValueRef {
 public:
  QKValueRef(qking_value_t value);
  virtual ~QKValueRef();
  inline qking_value_t get() { return value_; };

 private:
  qking_value_t value_;
  DISALLOW_COPY_AND_ASSIGN(QKValueRef);
};
RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_CORE_UTIL_H
