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

#ifndef QKING_ROOT_RAX_RENDER_BRIDGE_H
#define QKING_ROOT_RAX_RENDER_BRIDGE_H

#include <string>
#include "rax_common.h"
#include "rax_element.h"
#include "rax_text_element.h"

RAX_NAME_SPACE_BEGIN
namespace render_bridge {

// simple actions with out action posted to upper layer.
native_node_ptr NativeNodeCreate(RaxNativeElement* rax_element);

native_node_ptr NativeTextNodeCreate(RaxTextElement* rax_element);

void NativeNodeAddChildren(native_node_ptr parent, native_node_ptr child);

int32_t NativeNodeIndexOf(native_node_ptr parent, native_node_ptr child);

native_node_ptr NativeNodeChildOfIndex(native_node_ptr parent, uint32_t index);

native_node_ptr NativeNodeCreateRootNode();

void SetRootNode(native_node_ptr node);

// complex actions which post action to upper layer.

void RemoveNode(native_node_ptr node);

void InsertNode(native_node_ptr parent, native_node_ptr child, uint32_t index);

void MoveNode(native_node_ptr parent, native_node_ptr child, uint32_t index);

void UpdateAttr(native_node_ptr node,
                std::vector<std::pair<std::string, std::string>>* update_data);

void UpdateStyle(native_node_ptr node,
                 std::vector<std::pair<std::string, std::string>>* styles);

void AddEvent(native_node_ptr node, const std::string& event);

void RemoveEvent(native_node_ptr node, const std::string& event);

}  // namespace render_bridge
RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_RENDER_BRIDGE_H
