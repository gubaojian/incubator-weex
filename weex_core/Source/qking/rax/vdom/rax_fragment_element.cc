//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_fragment_element.h"
#include "rax_core_util.h"
#include "rax_element_factory.h"
#include "rax_update.h"

RAX_NAME_SPACE_BEGIN

RaxFragmentElement::RaxFragmentElement(uint32_t eid)
    : RaxElement(eid, RaxElementType::kFragement) {}

void RaxFragmentElement::MountComponentInternal(native_node_ptr render_parent,
                                                RaxElement *component_parent,
                                                const ChildMounter &mounter) {
  qking_value_t child = get_js_children();
  if (!qking_value_is_array(child)) {
    if (!qking_value_is_undefined(child)) {
      // only one.
      RaxElement *child_ele = get_factory()->GetRawTypeElement(child);
      if (child_ele == nullptr) {
        throw rax_common_err("Fragment MountChildren: Not a valid child type");
      }
      child_ele->MountComponent(render_parent, this, mounter);
      comp_children().push_back(child_ele);
    }
    // else { no children }
  } else {
    struct Passer {
      RaxFragmentElement *parent_this;
      native_node_ptr parent_native_node;
      bool error;
      const ChildMounter &child_mounter;
    } tmp{this, render_parent, false, mounter};
    qking_foreach_object_property_of(
        child,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void *user_data_p) -> bool {
          Passer *passer = (Passer *)user_data_p;
          RaxFragmentElement *parent_this = passer->parent_this;
          RaxElement *child_ele =
              parent_this->get_factory()->GetRawTypeElement(property_value);
          if (child_ele == nullptr) {
            passer->error = true;
            return false;
          }
          child_ele->MountComponent(passer->parent_native_node, parent_this,
                                    passer->child_mounter);
          parent_this->comp_children().push_back(child_ele);
          return true;
        },
        &tmp, true, true, false);

    if (tmp.error) {
      throw rax_common_err("Fragment MountChildren: Not a valid child type");
    }
  }
}
void RaxFragmentElement::UnmountComponentInternal(bool not_remove_child) {
  for (auto &it : comp_children()) {
    it->UnmountComponent(not_remove_child);
  }
}
void RaxFragmentElement::UpdateComponentInternal(RaxElement *prev_ele,
                                                 RaxElement *next_ele,
                                                 uint32_t insert_start) {
  RAX_ASSERT(get_comp_render_parent());

  UpdateComponentChildren(next_ele, this, get_comp_render_parent(),
                          insert_start);
}

void RaxFragmentElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  RAX_LOGD("RaxComponent(%u): Fragment, Children size: (%u)", eid(),
           static_cast<uint32_t>(comp_children().size()));
}
Json::object RaxFragmentElement::DebugCollectComponent() {
  Json::object obj = RaxElement::DebugCollectComponent();
  Json::array child_array;
  for (auto &it : comp_children()) {
    child_array.push_back(it->DebugCollectComponent());
  }
  if (!child_array.empty()) {
    obj["children"] = child_array;
  }
  return obj;
}
native_node_ptr RaxFragmentElement::get_first_render_node() {
  if (comp_children().empty()) {
    return nullptr;
  }
  RAX_ASSERT(comp_children().front());
  return comp_children().front()->get_first_render_node();
}
native_node_ptr RaxFragmentElement::get_last_render_node() {
  if (comp_children().empty()) {
    return nullptr;
  }
  RAX_ASSERT(comp_children().back());
  return comp_children().back()->get_first_render_node();
}

RAX_NAME_SPACE_END
