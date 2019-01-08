//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_element_factory.h"
#include "rax_class_element.h"
#include "rax_core_util.h"
#include "rax_empty_element.h"
#include "rax_fragment_element.h"
#include "rax_func_element.h"
#include "rax_native_element.h"
#include "rax_text_element.h"

RAX_NAME_SPACE_BEGIN

std::map<std::string, std::unique_ptr<RaxElementFactory>>
    RaxElementFactory::g_factories_{};

void RaxElementFactory::CreateFactory(const std::string &name) {
  RAX_MAKESURE(g_factories_.find(name) == g_factories_.end());

  g_factories_.insert(std::make_pair(
      name, std::unique_ptr<RaxElementFactory>(new RaxElementFactory(name))));
}

void RaxElementFactory::DestroyFactory(const std::string &name) {
  const auto &ptr = g_factories_.find(name);
  RAX_MAKESURE(ptr != g_factories_.end());
  g_factories_.erase(ptr);
}

RaxElementFactory *RaxElementFactory::GetFactory(const std::string &name) {
  const auto &ptr = g_factories_.find(name);
  RAX_MAKESURE(ptr != g_factories_.end());
  return ptr->second.get();
}

RaxElementFactory *RaxElementFactory::GetFactoryOrNull(
    const std::string &name) {
  const auto &ptr = g_factories_.find(name);
  if (ptr != g_factories_.end()) {
    return ptr->second.get();
  } else {
    return nullptr;
  }
}

RaxElementFactory::RaxElementFactory(const std::string &page_id)
    : page_id_(page_id) {}

RaxElementFactory::~RaxElementFactory() {
  // free pool
  ele_natives_.clear();
  elements_.clear();
  qking_release_value(document_event_handler_);
}

static bool is_component_class(qking_value_t type) {
  if (!qking_value_is_object(type)) {
    return false;
  }
  qking_value_t proto =
      qking_get_property_by_name_lit(type, QKING_LIT_MAGIC_STRING_EX_PROTOTYPE);
  if (qking_value_is_error(proto) || qking_value_is_undefined(proto)) {
    qking_release_value(proto);
    return false;
  }
  qking_value_t is_class = qking_get_property_by_name_lit(
      proto, QKING_LIT_MAGIC_STRING_EX_IS_COMPONENT_CLASS);
  qking_release_value(proto);

  if (qking_value_is_error(is_class)) {
    qking_release_value(is_class);
    return false;
  }
  if (qking_value_is_function(is_class)) {
    qking_release_value(is_class);
    return true;
  }
  qking_release_value(is_class);
  return false;
}

RaxElement *RaxElementFactory::CreateElement(const qking_value_t type,
                                             const qking_value_t props,
                                             const qking_value_t children,
                                             const qking_value_t key,
                                             const qking_value_t ref) {
  // todo stateless component not supported yet(has render, not a
  // isComponentClass)
  RaxElement *result = nullptr;
  if (qking_value_is_string(type)) {
    result = new RaxNativeElement(next_eid());
  } else if (is_component_class(type)) {
    result = new RaxClassElement(next_eid());
  } else if (qking_value_is_function(type)) {
    result = new RaxFuncElement(next_eid());
  }
  if (result) {
    result->set_factory(this);
    result->set_js_type(type);
    result->set_js_key(key);
    result->set_js_ref(ref);
    result->set_js_props(props);
    result->set_js_children(children);
    const auto &it = elements_.insert(
        std::make_pair(result->eid(), std::unique_ptr<RaxElement>(result)));
    RAX_ASSERT(it.second);
  }
  // make a element & return
  return result;
}

RaxElement *RaxElementFactory::GetRawTypeElement(const qking_value_t value) {
  RaxEleNativeType *native_ptr = nullptr;

  if (qking_get_object_native_pointer(value, (void **)&native_ptr, nullptr)) {
    return ExtractElement(native_ptr);
  }

  RaxElement *result = nullptr;
  if (qking_value_is_string(value) || qking_value_is_number(value)) {
    result = new RaxTextElement(next_eid(), string_from_qking_to_string(value));
  } else if (qking_value_is_array(value)) {
    result = new RaxFragmentElement(next_eid());
    result->set_js_children(value);
  } else if (qking_value_is_null_or_undefined(value) ||
             qking_value_is_boolean(value)) {
    result = new RaxEmptyElement(next_eid());
  }
  if (result) {
    result->set_factory(this);
    const auto &it = elements_.insert(
        std::make_pair(result->eid(), std::unique_ptr<RaxElement>(result)));
    RAX_ASSERT(it.second);
  }
  return result;
}

RaxRootElement *RaxElementFactory::CreateRootElement() {
  RaxRootElement *result = new RaxRootElement(next_eid());
  result->set_factory(this);
  RAX_ASSERT(elements_
                 .insert(std::make_pair(result->eid(),
                                        std::unique_ptr<RaxElement>(result)))
                 .second);
  return result;
}

RaxElement *RaxElementFactory::ExtractElement(RaxEleNativeType *native_t) {
  const auto &ptr = elements_.find(native_t->eid);
  if (ptr == elements_.end()) {
    // already released
    RAX_LOGE(
        "ExtractElement from RaxEleNativeType for an released "
        "RaxElement");
    return nullptr;
  }
  RaxElement *result = ptr->second.get();
  RAX_ASSERT(result == native_t->element);
  return result;
}

RaxElement *RaxElementFactory::ExtractElement(qking_value_t js_value) {
  RaxEleNativeType *native_ptr = nullptr;

  if (qking_get_object_native_pointer(js_value, (void **)&native_ptr,
                                      nullptr)) {
    return ExtractElement(native_ptr);
  }

  // not found;
  RAX_LOGE("ExtractElement from an invalid value");
  return nullptr;
}

RaxElement *RaxElementFactory::GetComponent(const std::string &ref) {
  char *ptr;
  const char *c_str = ref.c_str();
  unsigned long value = strtoul(c_str, &ptr, 10);
  if (c_str == ptr) {
    // error
    return nullptr;
  }
  const auto &it = elements_.find(static_cast<uint32_t>(value));
  if (it == elements_.end()) {
    return nullptr;
  }
  return it->second.get();
}

RaxEleNativeType *RaxElementFactory::MakeEleNativePtr(RaxElement *element) {
  RAX_ASSERT(element);

  std::unique_ptr<RaxEleNativeType> native_type(new RaxEleNativeType);
  //  RAX_LOGD("MakeEleNativePtr ptr: %lu", native_type.get());
  RaxEleNativeType *ptr = native_type.get();
  native_type->eid = element->eid();
  native_type->element = element;
  RAX_ASSERT(ele_natives_.find(native_type->eid) == ele_natives_.end());
  ele_natives_.insert(std::make_pair(
      native_type->eid,
      std::unique_ptr<RaxEleNativeType>(std::move(native_type))));
  return ptr;
}

static void ReleaseNativeType(void *ptr) {
  //  RAX_LOGD("ReleaseNativeType ptr: %lu", ptr);
  const std::string &name = rax_get_current_page_name();
  RAX_ASSERT(!name.empty());
  RaxElementFactory *factory = RaxElementFactory::GetFactoryOrNull(name);
  if (factory) {
    factory->ReleaseEleNativePtr((RaxEleNativeType *)ptr);
  }
}

static qking_object_native_info_t ReleaseInfo{ReleaseNativeType};

qking_value_t RaxElementFactory::MakeJSValue(RaxElement *element) {
  RAX_ASSERT(element);
  RaxEleNativeType *ptr = MakeEleNativePtr(element);
  RAX_ASSERT(ptr);
  qking_value_t ret = qking_create_object_native_pointer(ptr, &ReleaseInfo);
  element->set_js_ele(ret);
  return ret;
}

void RaxElementFactory::ReleaseEleNativePtr(RaxEleNativeType *native_t) {
  RAX_ASSERT(native_t);
  RAX_ASSERT(native_t->element);
  const auto &it = ele_natives_.find(native_t->eid);
  RAX_ASSERT(it != ele_natives_.end());
  RAX_ASSERT(native_t == it->second.get());
  ele_natives_.erase(it);
}

void RaxElementFactory::DebugPrintElements() {
  for (const auto &it : elements_) {
    it.second->DebugPrintElement();
  }
}

void RaxElementFactory::DebugPrintMountedComponent() {
  for (const auto &it : elements_) {
    if (it.second->comp_mounted()) {
      it.second->DebugPrintComponent();
    }
  }
}

void RaxElementFactory::CleanUpUnmountedComponent() {
  RAX_LOGD("===============CleanUpUnmountedComponent Start===============");

  auto it = elements_.begin();
  while (it != elements_.end()) {
    if (!it->second->comp_mounted())
      it = elements_.erase(it);
    else
      it++;
  }
  RAX_LOGD("===============CleanUpUnmountedComponent End===============\n");
}

void RaxElementFactory::set_document_event_handler(qking_value_t handler) {
  qking_release_value(document_event_handler_);
  document_event_handler_ = qking_acquire_value(handler);
}

RAX_NAME_SPACE_END
