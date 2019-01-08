//
// Created by Xu Jiacheng on 2018/12/25.
//

#include "rax_element.h"
#include "rax_core_util.h"

RAX_NAME_SPACE_BEGIN

#define T(name, string) string,
static const char *const s_names_[(int)RaxElementType::COUNT] = {
    RAX_ELE_TYPE_LIST(T)};
#undef T

const char *RaxElementTypeToString(RaxElementType type) {
  return s_names_[(int)type];
}

RaxElement::RaxElement(uint32_t eid, RaxElementType type)
    : eid_(eid), eid_str_(to_string(eid)), type_(type) {
  RAX_LOGD("RaxElement(%u) Newed, Type: %s", eid, RaxElementTypeToString(type));
}

RaxElement::~RaxElement() {
  RAX_LOGD("RaxElement(%u) Released, Type: %s", eid(),
           RaxElementTypeToString(type()));

  qking_release_value(js_type_);
  qking_release_value(js_props_);
  qking_release_value(js_children_);
  qking_release_value(js_key_);
  qking_release_value(js_ref_);
  qking_release_value(js_ele_ref_);
}

void RaxElement::set_js_type(qking_value_t type) {
  qking_release_value(js_type_);
  js_type_ = qking_acquire_value(type);
}

void RaxElement::set_js_props(qking_value_t props) {
  qking_release_value(js_props_);
  js_props_ = qking_acquire_value(props);
}

void RaxElement::set_js_children(qking_value_t children) {
  qking_release_value(js_children_);
  js_children_ = qking_acquire_value(children);
}

void RaxElement::set_js_key(qking_value_t key) {
  qking_release_value(js_key_);
  js_key_ = qking_acquire_value(key);
}

void RaxElement::set_js_ref(qking_value_t ref) {
  qking_release_value(js_ref_);
  js_ref_ = qking_acquire_value(ref);
}

void RaxElement::set_js_ele(qking_value_t ref) {
  qking_release_value(js_ele_ref_);
  js_ele_ref_ = qking_acquire_value(ref);
}

void RaxElement::CopyFromAnotherElement(RaxElement *element) {
  RAX_ASSERT(element);
  if (element == this) {
    return;
  }
  RAX_ASSERT(element->type() == type());
  RAX_ASSERT(element->get_factory() == get_factory());
  RAX_ASSERT(element->eid() != eid());

  set_js_type(element->get_js_type());
  set_js_props(element->get_js_props());
  set_js_key(element->get_js_key());
  set_js_ref(element->get_js_ref());
  set_js_children(element->get_js_children());

  // don't copy js_ele_ref_, it should stays the same
  // because classInstance._internal still points to this Component.
}

void RaxElement::MountComponent(native_node_ptr render_parent,
                                RaxElement *component_parent,
                                const ChildMounter &mounter) {
  RAX_ASSERT(render_parent);
  RAX_ASSERT(type() == RaxElementType::kRoot || component_parent);

  RAX_LOGD("RaxElement:%s Mount %u < %i%s", RaxElementTypeToString(type()),
           eid(), component_parent ? component_parent->eid() : -1,
           mounter ? " ,has Mounter" : "");
  set_comp_render_parent(render_parent);
  set_comp_parent(component_parent);
  try {
    MountComponentInternal(render_parent, component_parent, mounter);
  } catch (const rax_common_err &e) {
    throw e;
  }
  comp_mounted_ = true;
}

void RaxElement::UnmountComponent(bool not_remove_child) {
  RAX_ASSERT(comp_mounted_);
  RAX_LOGD("RaxComponent(%u):%s UnmountComponent remove_child: %s", eid(),
           RaxElementTypeToString(type()), not_remove_child ? "false" : "true");
  UnmountComponentInternal(not_remove_child);
  comp_mounted_ = false;
}

void RaxElement::UpdateComponent(RaxElement *prev_ele, RaxElement *next_ele,
                                 uint32_t insert_start) {
  RAX_ASSERT(prev_ele);
  RAX_ASSERT(next_ele);
  RAX_LOGD(
      "RaxComponent(%u):%s UpdateComponent prev(%u) -> next(%u), insert_start: "
      "%u",
      eid(), RaxElementTypeToString(type()), prev_ele->eid(), next_ele->eid(),
      insert_start);

  RAX_ASSERT(prev_ele == this);
  // component.type aka element.type can't change.
  RAX_ASSERT(next_ele->type() == type());
  UpdateComponentInternal(prev_ele, next_ele, insert_start);
}

void RaxElement::MountComponentInternal(native_node_ptr render_parent,
                                        RaxElement *component_parent,
                                        const ChildMounter &mounter) {
  RAX_LOGE("Not support MountComponentInternal for: <%s>",
           RaxElementTypeToString(type()));
}

void RaxElement::UpdateComponentInternal(RaxElement *prev_ele,
                                         RaxElement *next_ele,
                                         uint32_t insert_start) {
  RAX_LOGE("Not support UpdateComponentInternal for: <%s>",
           RaxElementTypeToString(type()));
}

void RaxElement::UnmountComponentInternal(bool not_remove_child) {
  RAX_LOGE("Not support UnmountComponentInternal for: <%s>",
           RaxElementTypeToString(type()));
}

void RaxElement::DebugPrintElement() {
  RAX_LOGD(
      "RaxElement(%u) type: %s, key: %s, ref: %s\n"
      "\t\tProps: %s\n"
      "\t\tChildren: %s",
      eid(),
      (std::string(RaxElementTypeToString(type())) + "(" +
       string_from_qking_to_string(get_js_type()) + ")")
          .c_str(),
      string_from_qking_to_string(get_js_key()).c_str(),
      (qking_value_is_string(get_js_ref())
           ? string_from_qking_to_string(get_js_ref())
           : string_from_qking_to_string(get_js_ref()))
          .c_str(),
      string_from_qking_json_stringify(get_js_props()).c_str(),
      string_from_qking_to_string(get_js_children()).c_str());
}

void RaxElement::DebugPrintComponent() {}

Json::object RaxElement::DebugCollectComponent() {
  Json::object object;

  object["0_id"] = static_cast<int>(eid());
  object["1_type"] = RaxElementTypeToString(type());
  if (!qking_value_is_undefined(get_js_key())) {
    object["2_key"] = string_from_qking_to_string(get_js_key());
  }
  if (qking_value_is_string(get_js_ref())) {
    object["3_ref"] = string_from_qking_to_string(get_js_ref());
  }

  return object;
}

RAX_NAME_SPACE_END
