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

#ifndef QKING_ROOT_RAXELEMENT_H
#define QKING_ROOT_RAXELEMENT_H

#include <vector>
#include "rax_common.h"

RAX_NAME_SPACE_BEGIN

#define RAX_ELE_TYPE_LIST(T) \
  T(kNative, "Native")       \
  T(kClass, "Class")         \
  T(kFunction, "Function")   \
  T(kFragement, "Fragement") \
  T(kText, "Text")           \
  T(kRoot, "Root")           \
  T(kEmpty, "Empty")

#define T(name, string) name,
enum class RaxElementType { RAX_ELE_TYPE_LIST(T) COUNT };
#undef T

const char *RaxElementTypeToString(RaxElementType type);

class RaxNativeElement;
class RaxElementFactory;

using ChildMounter =
    std::function<void(native_node_ptr parent, native_node_ptr child)>;

class RaxElement {
 public:
  RaxElement(uint32_t eid, RaxElementType type);

  virtual ~RaxElement();

  inline uint32_t eid() { return eid_; }

  inline RaxElementFactory *get_factory() { return factory_; }

  inline void set_factory(RaxElementFactory *factory) { factory_ = factory; }

  inline const std::string &eid_str() const { return eid_str_; }

  inline RaxElementType type() { return type_; }

  void set_js_type(qking_value_t type);

  inline qking_value_t get_js_type() { return js_type_; };

  void set_js_props(qking_value_t props);

  inline qking_value_t get_js_props() { return js_props_; };

  void set_js_children(qking_value_t children);

  inline qking_value_t get_js_children() { return js_children_; }

  void set_js_key(qking_value_t key);

  inline qking_value_t get_js_key() { return js_key_; }

  void set_js_ref(qking_value_t ref);

  inline qking_value_t get_js_ref() { return js_ref_; }

  inline qking_value_t get_js_ele() { return js_ele_ref_; }

  void set_js_ele(qking_value_t element_obj);

  inline std::vector<RaxElement *> &comp_children() { return comp_children_; }

  inline RaxElement *get_comp_parent() { return comp_parent_; }

  inline native_node_ptr get_comp_render_parent() {
    return comp_render_parent_;
  }

  virtual native_node_ptr get_first_render_node() = 0;

  virtual native_node_ptr get_last_render_node() = 0;

  inline void set_comp_parent(RaxElement *parent) { comp_parent_ = parent; }

  inline void set_comp_render_parent(native_node_ptr parent) {
    comp_render_parent_ = parent;
  }

  inline bool comp_mounted() { return comp_mounted_; }

  void CopyFromAnotherElement(RaxElement *element);

  void DebugPrintElement();

  void MountComponent(native_node_ptr render_parent,
                      RaxElement *component_parent,
                      const ChildMounter &mounter = nullptr);

  void UpdateComponent(RaxElement *prev_ele, RaxElement *next_ele,
                       uint32_t insert_start);

  void UnmountComponent(bool not_remove_child);

  virtual void DebugPrintComponent();

  virtual Json::object DebugCollectComponent();

 protected:
  virtual void UnmountComponentInternal(bool not_remove_child = false);

  virtual void MountComponentInternal(native_node_ptr render_parent,
                                      RaxElement *component_parent,
                                      const ChildMounter &mounter);
  virtual void UpdateComponentInternal(RaxElement *prev_ele,
                                       RaxElement *next_ele,
                                       uint32_t insert_start);

 private:
  uint32_t eid_ = 0;
  std::string eid_str_;
  RaxElementFactory *factory_ = nullptr;
  RaxElementType type_ = RaxElementType::kEmpty;

  /*==================as element==========================*/
  qking_value_t js_type_ = qking_create_undefined();
  qking_value_t js_props_ = qking_create_undefined();
  qking_value_t js_key_ = qking_create_undefined();
  qking_value_t js_ref_ = qking_create_undefined();

  // increase ref count. point to this NativeComponent Object in js. used
  // to pass this value to ClassInstance.
  qking_value_t js_ele_ref_ = qking_create_undefined();

  //.children is set up after <Tag> createElement Called;
  qking_value_t js_children_ = qking_create_undefined();
  /*=======================================================*/

  /*==================as component==========================*/
  std::vector<RaxElement *> comp_children_;
  RaxElement *comp_parent_ = nullptr;
  native_node_ptr comp_render_parent_ = nullptr;
  bool comp_mounted_ = false;

  /*=======================================================*/
  DISALLOW_COPY_AND_ASSIGN(RaxElement);
};

RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAXELEMENT_H
