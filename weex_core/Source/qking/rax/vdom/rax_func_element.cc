//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_func_element.h"
#include "rax_core_util.h"
#include "rax_element_factory.h"
#include "rax_render_bridge.h"
#include "rax_update.h"

RAX_NAME_SPACE_BEGIN

RaxFuncElement::RaxFuncElement(uint32_t eid)
    : RaxElement(eid, RaxElementType::kFunction) {}

RaxElement *RaxFuncElement::CallRenderOnInstance() {
  // render
  qking_value_t args[1] = {get_js_props()};
  qking_value_t ret =
      qking_call_function(get_js_type(), qking_create_undefined(), args, 1);

  if (qking_value_is_error(ret)) {
    const std::string &err_msg = string_from_qking_error(ret);
    qking_release_value(ret);
    std::string final_msg =
        "FuncElement MountComponent: Function Element render exception, ";
    final_msg.append(err_msg);
    throw rax_common_err(final_msg);
  }

  RaxElement *rendered = get_factory()->GetRawTypeElement(ret);
  if (!rendered) {
    qking_release_value(ret);
    throw rax_common_err("FuncElement MountComponent: Invalid return type");
  }

  qking_release_value(ret);
  return rendered;
}

void RaxFuncElement::MountComponentInternal(native_node_ptr render_parent,
                                            RaxElement *component_parent,
                                            const ChildMounter &mounter) {
  // render
  RaxElement *rendered = CallRenderOnInstance();

  set_rendered_comp(rendered);
  rendered->MountComponent(render_parent, this, mounter);
}
void RaxFuncElement::UnmountComponentInternal(bool not_remove_child) {
  RAX_ASSERT(get_rendered_comp());
  get_rendered_comp()->UnmountComponent(not_remove_child);
}

void RaxFuncElement::UpdateComponentInternal(RaxElement *prev_ele,
                                             RaxElement *next_ele,
                                             uint32_t insert_start) {
  // only called by setState, could prev_ele == next_ele;
  RAX_ASSERT((prev_ele != next_ele) || g_by_state--);

  RAX_ASSERT(
      qking_value_strict_equal(this->get_js_type(), next_ele->get_js_type()));
  RAX_ASSERT(
      qking_value_strict_equal(this->get_js_key(), next_ele->get_js_key()));

  // using next ele content to replace current
  CopyFromAnotherElement(next_ele);

  // render
  UpdateRenderedComponent(insert_start);
}

void RaxFuncElement::UpdateRenderedComponent(uint32_t insert_start) {
  RaxElement *prev_rendered_ele = get_rendered_comp();

  RaxElement *next_rendered_ele = CallRenderOnInstance();

  bool should_update =
      ShouldUpdateComponent(prev_rendered_ele, next_rendered_ele);

  if (should_update) {
    if (prev_rendered_ele != next_rendered_ele) {
      prev_rendered_ele->UpdateComponent(prev_rendered_ele, next_rendered_ele,
                                         insert_start);
    }
  } else {
    prev_rendered_ele->UnmountComponent(false);
    native_node_ptr render_parent = get_comp_render_parent();
    next_rendered_ele->MountComponent(
        render_parent, this,
        [&insert_start, render_parent](native_node_ptr parent,
                                       native_node_ptr child) {
          RAX_ASSERT(parent && child);
          RAX_ASSERT(render_parent == parent);

          render_bridge::InsertNode(parent, child, insert_start);
          insert_start++;
        });
    set_rendered_comp(next_rendered_ele);
  }
}

void RaxFuncElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  RAX_LOGD("RaxComponent(%u): Function, Rendered: (%u)", eid(),
           get_rendered_comp()->eid());
}
Json::object RaxFuncElement::DebugCollectComponent() {
  Json::object obj = RaxElement::DebugCollectComponent();
  obj["child"] = get_rendered_comp()->DebugCollectComponent();
  return obj;
}
native_node_ptr RaxFuncElement::get_first_render_node() {
  RAX_ASSERT(get_rendered_comp());
  return get_rendered_comp()->get_first_render_node();
}
native_node_ptr RaxFuncElement::get_last_render_node() {
  RAX_ASSERT(get_rendered_comp());
  return get_rendered_comp()->get_last_render_node();
}
RAX_NAME_SPACE_END
