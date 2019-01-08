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

#ifndef QKING_ROOT_RAX_NATIVE_ELEMENT_H
#define QKING_ROOT_RAX_NATIVE_ELEMENT_H

#include "rax_common.h"
#include "rax_core_util.h"
#include "rax_element.h"

RAX_NAME_SPACE_BEGIN

class RaxNativeElement : public RaxElement {
 public:
  RaxNativeElement(uint32_t eid);
  virtual ~RaxNativeElement();

  inline void set_style(const std::string &key, const std::string &value) {
    comp_styles_[key] = value;
  }

  inline void set_attr(const std::string &key, const std::string &value) {
    comp_attrs_[key] = value;
  }

  inline void set_event(const std::string &key, qking_value_t value) {
    comp_events_[key] = std::unique_ptr<QKValueRef>(new QKValueRef(value));
  }

  inline void remove_event(const std::string &key) { comp_events_.erase(key); }

  inline const std::map<std::string, std::string> &get_styles() {
    return comp_styles_;
  }

  inline const std::map<std::string, std::string> &get_attrs() {
    return comp_attrs_;
  }

  inline std::map<std::string, std::unique_ptr<QKValueRef>> &get_events() {
    return comp_events_;
  }

  inline native_node_ptr native_node() { return comp_native_node_; }

  inline std::string &comp_type() { return comp_type_; }

  native_node_ptr get_first_render_node() override;

  native_node_ptr get_last_render_node() override;

  void DebugPrintComponent() override;
  Json::object DebugCollectComponent() override;

 protected:
  void MountComponentInternal(native_node_ptr render_parent,
                              RaxElement *component_parent,
                              const ChildMounter &mounter) override;

  void UnmountComponentInternal(bool not_remove_child) override;

  void UpdateComponentInternal(RaxElement *prev_ele, RaxElement *next_ele,
                               uint32_t insert_start) override;

 private:
  void SetNativeCompProps();

  void MountChildren();

  bool MountChildrenForText();

  bool UpdateComponentChildrenOfText(RaxElement *next_ele,
                                     uint32_t insert_start_idx);

  void UpdateProperties(RaxNativeElement *next_ele);

  void UpdateChildren(RaxNativeElement *next_ele);

 private:
  std::string comp_type_;
  std::map<std::string, std::string> comp_styles_;
  std::map<std::string, std::string> comp_attrs_;
  std::map<std::string, std::unique_ptr<QKValueRef>> comp_events_;

  native_node_ptr comp_native_node_ = nullptr;
};
RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_NATIVE_ELEMENT_H
