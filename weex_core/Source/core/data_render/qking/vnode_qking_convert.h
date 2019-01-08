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

#ifndef CORE_DATA_RENDER_VNODE_QKING_CONVERT_H
#define CORE_DATA_RENDER_VNODE_QKING_CONVERT_H

#include <string>
#include "qking/include/qking.h"

namespace weex {
namespace core {
namespace data_render {

bool qking_value_deep_compare(const qking_value_t left,const qking_value_t right);

bool qking_value_add_all_to(const qking_value_t src,const qking_value_t dest);

std::string string_from_qking_string_value(const qking_value_t var);
    
std::string string_from_qking_get_property_by_name(const qking_value_t obj_val, const char *name_p);
    
std::string string_from_qking_json_stringify(const qking_value_t object_to_stringify);
    
std::string string_from_qking_get_property_by_index(const qking_value_t obj_val, uint32_t index);

std::string string_from_qking_error(const qking_value_t err);

bool string_from_qking_execute_code(qking_executor_t executor, std::string &error);
    
bool string_from_qking_set_assembly_code(qking_executor_t executor, uint8_t *code, size_t size, std::string &error);
    
#ifdef DEBUG
std::string qking_value_print(const qking_value_t value);
#endif

}  // namespace data_render
}  // namespace core
}  // namespace weex

#endif  // CORE_DATA_RENDER_VNODE_QKING_CONVERT_H
