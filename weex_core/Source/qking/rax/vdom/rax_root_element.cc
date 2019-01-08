//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_root_element.h"
#include "rax_core_util.h"

RAX_NAME_SPACE_BEGIN

RaxRootElement::RaxRootElement(uint32_t eid)
    : RaxElement(eid, RaxElementType::kRoot) {}

void RaxRootElement::MountComponentInternal(native_node_ptr render_parent,
                                            RaxElement *component_parent,
                                            const ChildMounter &mounter) {
  // forward to child
  RAX_ASSERT(child());
  child()->MountComponent(render_parent, this, mounter);
}

void RaxRootElement::UnmountComponentInternal(bool not_remove_child) {
  RAX_ASSERT(false);
}

void RaxRootElement::UpdateComponentInternal(RaxElement *prev_ele,
                                             RaxElement *next_ele,
                                             uint32_t insert_start) {
  RAX_ASSERT(false);
}

void RaxRootElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  RAX_LOGD("RaxComponent(%u): Root <Root>", eid());
}
Json::object RaxRootElement::DebugCollectComponent() {
  Json::object obj = RaxElement::DebugCollectComponent();
  obj["child"] = child()->DebugCollectComponent();
  return obj;
}
native_node_ptr RaxRootElement::get_first_render_node() {
  RAX_ASSERT(child());
  return child()->get_first_render_node();
}
native_node_ptr RaxRootElement::get_last_render_node() {
  RAX_ASSERT(child());
  return child()->get_last_render_node();
}
RAX_NAME_SPACE_END
