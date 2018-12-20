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

#ifndef QKING_ROOT_VM_ECMA_VALUE_HELPER_H
#define QKING_ROOT_VM_ECMA_VALUE_HELPER_H

#include "core/ecma/base/ecma_globals.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define QKING_TO_LOG_STR_START(str_name, value) \
  const char* str_name = qking_convert_to_log_str_from_value(value);

#define QKING_TO_LOG_STR_FINISH(str_name) jmem_system_free((void*)str_name)

const char* qking_convert_to_log_str_from_value(ecma_value_t value);

#define QKING_GET_LOG_STR_START(str_name, value) \
  const char* str_name = qking_get_log_str_from_ecma_string(value);

#define QKING_GET_LOG_STR_FINISH(str_name) jmem_system_free((void*)str_name)

const char* qking_get_log_str_from_ecma_string(ecma_string_t* value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // QKING_ROOT_VM_ECMA_VALUE_HELPER_H
