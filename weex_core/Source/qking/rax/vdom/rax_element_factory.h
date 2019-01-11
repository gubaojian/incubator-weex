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

#ifndef QKING_ROOT_RAX_ELEMENT_FACTORY_H
#define QKING_ROOT_RAX_ELEMENT_FACTORY_H

#include <map>
#include <memory>
#include <string>
#include "rax_common.h"
#include "rax_element.h"
#include "rax_root_element.h"

RAX_NAME_SPACE_BEGIN

struct RaxEleNativeType {
  uint32_t eid;
  RaxElement* element;
};

class RaxElementFactory {
 public:
  static void CreateFactory(const std::string& name);
  static void DestroyFactory(const std::string& name);
  static RaxElementFactory* GetFactory(const std::string& name);
  static RaxElementFactory* GetFactoryOrNull(const std::string& name);

 public:
  RaxElementFactory(const std::string& page_id);
  virtual ~RaxElementFactory();

  inline const std::string& page_id() const { return page_id_; }
  inline uint32_t next_eid() { return element_id_++; }

  inline RaxElement*& root() { return root_ele_; }

  void set_document_event_handler(qking_value_t handler);

  // no need to release
  inline qking_value_t get_docuemnt_event_handler() {
    return document_event_handler_;
  };

  /**
   * @return null if type err;
   */
  RaxElement* CreateElement(const qking_value_t type, const qking_value_t props,
                            const qking_value_t children,
                            const qking_value_t key, const qking_value_t ref);

  RaxRootElement* CreateRootElement();

  /**
   * @return null if type err;
   */
  RaxElement* GetRawTypeElement(const qking_value_t type);

  /**
   * @return null if already released
   */
  RaxElement* ExtractElement(RaxEleNativeType* native_t);

  /**
   * @return null if already released
   */
  RaxElement* ExtractElement(qking_value_t js_value);

  /**
   * @param element none null
   * @return none null
   */
  RaxEleNativeType* MakeEleNativePtr(RaxElement* element);

  /**
   * @param element none null
   * @return native ptr js value.
   */
  qking_value_t MakeJSValue(RaxElement* element);

  RaxElement* GetComponent(const std::string& ref);

  void ReleaseEleNativePtr(RaxEleNativeType* native_t);

  void CleanUpUnmountedComponent();

  void DebugPrintElements();

  void DebugPrintMountedComponent();

 private:
  //static object got release on app terminate in IOS. Using ptr to avoid this issue.
  static std::map<std::string, std::unique_ptr<RaxElementFactory>>* g_factories_;

 private:
  uint32_t element_id_ = 0;
  std::string page_id_;
  RaxElement* root_ele_ = nullptr;
  qking_value_t document_event_handler_ = qking_create_undefined();
  std::map<uint32_t, std::unique_ptr<RaxElement>> elements_;
  std::map<uint32_t, std::unique_ptr<RaxEleNativeType>> ele_natives_;
};

RAX_NAME_SPACE_END

#endif  // QKING_ROOT_RAX_ELEMENT_FACTORY_H
