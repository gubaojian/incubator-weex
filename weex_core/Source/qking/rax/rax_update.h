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

#ifndef QKING_ROOT_RAX_UPDATE_H
#define QKING_ROOT_RAX_UPDATE_H

#include "rax_common.h"
#include "rax_core_util.h"
#include "rax_element.h"

RAX_NAME_SPACE_BEGIN

extern uint32_t g_by_state;

template <typename T>
uint32_t find_and_remove(std::vector<T> &container, const T &value) {
  const auto &it = std::find(container.begin(), container.end(), value);
  RAX_ASSERT(it != container.end());
  container.erase(it);
  return std::distance(container.begin(), it);
}

template <typename T>
uint32_t find_index_of(const std::vector<T> &container, const T &value) {
  const auto &it = std::find(container.begin(), container.end(), value);
  RAX_ASSERT(it != container.end());
  return static_cast<uint32_t>(std::distance(container.begin(), it));
}

template <typename T, typename V>
void find_and_remove(std::map<T, V> &container, const T &value) {
  const auto &it = container.find(value);
  RAX_ASSERT(it != container.end());
  container.erase(it);
}

bool ShouldUpdateComponent(RaxElement *prev, RaxElement *next);

void UpdateNodeProperties(RaxNativeElement *old_node,
                          RaxNativeElement *new_node);

void PrepareChildrenElement(RaxElement *next_ele);

void UpdateComponentChildren(RaxElement *next_ele, RaxElement *current_ele,
                             native_node_ptr parent_node,
                             uint32_t insert_start_idx);

std::string GetKeyNameOfElement(std::map<std::string, RaxElement *> &map,
                                RaxElement *element, uint32_t index);

RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_UPDATE_H
