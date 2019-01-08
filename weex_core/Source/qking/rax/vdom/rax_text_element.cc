//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_text_element.h"
#include "rax_core_util.h"
#include "rax_render_bridge.h"

RAX_NAME_SPACE_BEGIN

using namespace render_bridge;

RaxTextElement::RaxTextElement(uint32_t eid, const std::string &text)
    : text_(text), RaxElement(eid, RaxElementType::kText) {}

void RaxTextElement::MountComponentInternal(native_node_ptr render_parent,
                                            RaxElement *component_parent,
                                            const ChildMounter &mounter) {
  // text element can't be mount under Native<text> node.
  RAX_ASSERT(component_parent->type() != RaxElementType::kText);
  // create native
  comp_native_node_ = NativeTextNodeCreate(this);
  RAX_ASSERT(comp_native_node_);

  if (mounter) {
    mounter(render_parent, native_node());
  } else {
    NativeNodeAddChildren(render_parent, native_node());
  }
}

void RaxTextElement::UnmountComponentInternal(bool not_remove_child) {
  RAX_ASSERT(comp_native_node_);
  if (not_remove_child) {
    return;
  }
  RemoveNode(comp_native_node_);
}

void RaxTextElement::UpdateComponentInternal(RaxElement *prev_ele,
                                             RaxElement *next_ele,
                                             uint32_t insert_start) {
  RaxTextElement *next_txt = dynamic_cast<RaxTextElement *>(next_ele);
  const std::string &new_str = next_txt->text();

  if (new_str == text_) {
    return;
  }

  text_ = new_str;
  std::vector<std::pair<std::string, std::string>> *p_vec =
      new std::vector<std::pair<std::string, std::string>>{{"value", text_}};
  UpdateAttr(comp_native_node_, p_vec);
}

void RaxTextElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  RAX_LOGD("RaxComponent(%u): Text \"%s\"", eid(), text().c_str());
}

Json::object RaxTextElement::DebugCollectComponent() {
  Json::object obj = RaxElement::DebugCollectComponent();
  obj["text"] = text();
  return obj;
}
native_node_ptr RaxTextElement::get_first_render_node() {
  RAX_ASSERT(comp_native_node_);
  return comp_native_node_;
}
native_node_ptr RaxTextElement::get_last_render_node() {
  RAX_ASSERT(comp_native_node_);
  return comp_native_node_;
}
RAX_NAME_SPACE_END
