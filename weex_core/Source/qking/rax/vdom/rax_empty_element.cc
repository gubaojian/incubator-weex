//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_empty_element.h"
#include "rax_core_util.h"

RAX_NAME_SPACE_BEGIN

void RaxEmptyElement::MountComponentInternal(native_node_ptr render_parent,
                                             RaxElement *component_parent,
                                             const ChildMounter &mounter) {
  // don't do anything, skip;
}
RaxEmptyElement::RaxEmptyElement(uint32_t eid)
    : RaxElement(eid, RaxElementType::kEmpty) {
  RAX_LOGW("Create an empty element %u", eid);
}
void RaxEmptyElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  RAX_LOGD("RaxComponent(%u): Empty", eid());
}
native_node_ptr RaxEmptyElement::get_first_render_node() { return nullptr; }
native_node_ptr RaxEmptyElement::get_last_render_node() { return nullptr; }

void RaxEmptyElement::UnmountComponentInternal(bool not_remove_child) {
  // don't do anything, skip;
}

void RaxEmptyElement::UpdateComponentInternal(RaxElement *prev_ele,
                                              RaxElement *next_ele,
                                              uint32_t insert_start) {
  // don't do anything, skip;
}

RAX_NAME_SPACE_END