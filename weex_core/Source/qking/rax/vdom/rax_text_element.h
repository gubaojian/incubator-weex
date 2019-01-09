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
// Created by Xu Jiacheng on 2018/12/26.
//

#ifndef QKING_ROOT_RAX_TEXT_ELEMENT_H
#define QKING_ROOT_RAX_TEXT_ELEMENT_H

#include "rax_common.h"
#include "rax_element.h"

RAX_NAME_SPACE_BEGIN

class RaxTextElement : public RaxElement {
 public:
  RaxTextElement(uint32_t eid, const std::string &text);

  inline const std::string &text() { return text_; }

  inline native_node_ptr native_node() { return comp_native_node_; }

  void DebugPrintComponent() override;

  Json::object DebugCollectComponent() override;

  native_node_ptr get_first_render_node() override;
  native_node_ptr get_last_render_node() override;

 protected:
  void MountComponentInternal(native_node_ptr render_parent,
                              RaxElement *component_parent,
                              const ChildMounter &mounter) override;
  void UnmountComponentInternal(bool not_remove_child) override;
  void UpdateComponentInternal(RaxElement *prev_ele, RaxElement *next_ele,
                               uint32_t insert_start) override;

 private:
  native_node_ptr comp_native_node_ = nullptr;

  std::string text_;
};
RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_TEXT_ELEMENT_H