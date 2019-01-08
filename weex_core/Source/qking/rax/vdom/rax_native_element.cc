//
// Created by Xu Jiacheng on 2018/12/26.
//

#include "rax_native_element.h"
#include <numeric>
#include <regex>
#include "rax_element_factory.h"
#include "rax_render_bridge.h"
#include "rax_update.h"

RAX_NAME_SPACE_BEGIN
using namespace render_bridge;

RaxNativeElement::~RaxNativeElement() {}

RaxNativeElement::RaxNativeElement(uint32_t eid)
    : RaxElement(eid, RaxElementType::kNative) {}

void RaxNativeElement::MountComponentInternal(native_node_ptr render_parent,
                                              RaxElement *component_parent,
                                              const ChildMounter &mounter) {
  comp_type() = string_from_qking_to_string(get_js_type());
  SetNativeCompProps();
  // text has different behavior
  bool is_text_node = false;
  if (MountChildrenForText()) {
    is_text_node = true;
  }

  // create native
  comp_native_node_ = NativeNodeCreate(this);
  RAX_ASSERT(comp_native_node_);

  if (!is_text_node) {
    MountChildren();
  }

  if (mounter) {
    mounter(render_parent, native_node());
  } else {
    NativeNodeAddChildren(render_parent, native_node());
  }
}

bool RaxNativeElement::MountChildrenForText() {
  if (comp_type() == "text") {
    // collect children;

    qking_value_t child = get_js_children();
    if (!qking_value_is_array(child)) {
      if (!qking_value_is_undefined(child)) {
        // only one.
        RaxElement *child_ele = get_factory()->GetRawTypeElement(child);
        if (child_ele == nullptr) {
          throw rax_common_err("Native MountChildren: Not a valid child type");
        }
        if (child_ele->type() == RaxElementType::kText) {
          set_attr("value", dynamic_cast<RaxTextElement *>(child_ele)->text());
        } /* else { skip; } */
      }
      // else { no children }
    } else {
      struct Passer {
        RaxNativeElement *parent_this;
        std::string all_string;
        bool has_text_child;
        bool error;
      } tmp{this, "", false, false};
      qking_foreach_object_property_of(
          child,
          [](const qking_value_t property_name,
             const qking_value_t property_value, void *user_data_p) -> bool {
            Passer *passer = (Passer *)user_data_p;
            RaxNativeElement *parent_this = passer->parent_this;
            RaxElement *child_ele =
                parent_this->get_factory()->GetRawTypeElement(property_value);
            if (child_ele == nullptr) {
              passer->error = true;
              return false;
            }
            if (child_ele->type() == RaxElementType::kText) {
              passer->all_string +=
                  dynamic_cast<RaxTextElement *>(child_ele)->text();
              passer->has_text_child = true;
            } /* else { skip; } */
            return true;
          },
          &tmp, true, true, false);

      if (tmp.error) {
        throw rax_common_err("Native MountChildren: Not a valid child type");
      }
      if (tmp.has_text_child) {
        set_attr("value", tmp.all_string);
      }
    }

    return true;
  } else {
    return false;
  }
}

void RaxNativeElement::MountChildren() {
  qking_value_t child = get_js_children();
  if (!qking_value_is_array(child)) {
    if (!qking_value_is_undefined(child)) {
      // only one.
      RaxElement *child_ele = get_factory()->GetRawTypeElement(child);
      if (child_ele == nullptr) {
        throw rax_common_err("Native MountChildren: Not a valid child type");
      }
      child_ele->MountComponent(native_node(), this, nullptr);
      comp_children().push_back(child_ele);
    }
    // else { no children }
  } else {
    struct Passer {
      RaxNativeElement *parent_this;
      bool error;
    } tmp{this, false};
    qking_foreach_object_property_of(
        child,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void *user_data_p) -> bool {
          Passer *passer = (Passer *)user_data_p;
          RaxNativeElement *parent_this = passer->parent_this;
          RaxElement *child_ele =
              parent_this->get_factory()->GetRawTypeElement(property_value);
          if (child_ele == nullptr) {
            passer->error = true;
            return false;
          }
          child_ele->MountComponent(parent_this->native_node(), parent_this,
                                    nullptr);
          parent_this->comp_children().push_back(child_ele);
          return true;
        },
        &tmp, true, true, false);

    if (tmp.error) {
      throw rax_common_err("Native MountChildren: Not a valid child type");
    }
  }
}

void RaxNativeElement::SetNativeCompProps() {
  qking_value_t props = get_js_props();

  // style
  qking_value_t style =
      qking_get_property_by_name_lit(props, QKING_LIT_MAGIC_STRING_EX_STYLE);
  if (!qking_value_is_error(style)) {
    // travel style;
    qking_foreach_object_property_of(
        style,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void *user_data_p) -> bool {
          if (!qking_value_is_string(property_name)) {
            return true;
          }
          std::string style_key = string_from_qking_string_value(property_name);
          std::string style_str;
          if (qking_value_is_null_or_undefined(property_value)) {
            style_str = "";
          } else if (!qking_value_is_string(property_value)) {
            style_str = string_from_qking_to_string(property_value);
          } else {
            style_str = string_from_qking_string_value(property_value);
          }

          RaxNativeElement *rax_element = (RaxNativeElement *)user_data_p;
          rax_element->set_style(style_key, style_str);

          return true;
        },
        this, false, true, false);
  }
  qking_release_value(style);

  // attr & event
  if (qking_value_is_object(props)) {
    // travel style;
    qking_foreach_object_property_of(
        props,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void *user_data_p) -> bool {
          static std::regex EVENT_REG("^on[A-Z]");

          if (!qking_value_is_string(property_name)) {
            return true;
          }
          std::string props_key = string_from_qking_string_value(property_name);
          if (props_key == "children" || props_key == "style") {
            // skip children and style
            return true;
          }
          RaxNativeElement *rax_element = (RaxNativeElement *)user_data_p;
          if (std::regex_search(props_key, EVENT_REG)) {
            // a event
            std::string event_key = props_key.substr(2, props_key.size());
            std::transform(event_key.begin(), event_key.end(),
                           event_key.begin(), ::tolower);
            rax_element->set_event(event_key, property_value);
          } else {
            std::string props_value;
            if (qking_value_is_null_or_undefined(property_value)) {
              props_value = "";
            } else if (!qking_value_is_string(property_value)) {
              props_value = string_from_qking_to_string(property_value);
            } else {
              props_value = string_from_qking_string_value(property_value);
            }
            rax_element->set_attr(props_key, props_value);
          }
          return true;
        },
        this, false, true, false);
  }
}

void RaxNativeElement::UnmountComponentInternal(bool not_remove_child) {
  RAX_ASSERT(comp_native_node_);
  if (!not_remove_child) {
    RemoveNode(comp_native_node_);
  }

  for (auto &it : comp_children()) {
    it->UnmountComponent(true);  // parent or higher parent has been removed.
  }
}
void RaxNativeElement::UpdateComponentInternal(RaxElement *prev_ele,
                                               RaxElement *next_ele,
                                               uint32_t insert_start) {
  RAX_ASSERT(prev_ele != next_ele);
  RAX_ASSERT(
      qking_value_strict_equal(this->get_js_type(), next_ele->get_js_type()));
  RAX_ASSERT(
      qking_value_strict_equal(this->get_js_key(), next_ele->get_js_key()));

  RaxNativeElement *next_native = dynamic_cast<RaxNativeElement *>(next_ele);

  UpdateProperties(next_native);
  UpdateChildren(next_native);

  CopyFromAnotherElement(next_ele);
}

void RaxNativeElement::UpdateProperties(RaxNativeElement *next_ele) {
  // prepare native props to compare.
  next_ele->SetNativeCompProps();

  UpdateNodeProperties(this, next_ele);
}

bool RaxNativeElement::UpdateComponentChildrenOfText(
    RaxElement *next_ele, uint32_t insert_start_idx) {
  if (comp_type() == "text") {
    RAX_ASSERT(next_ele->type() == RaxElementType::kNative);
    RaxNativeElement *next_native = dynamic_cast<RaxNativeElement *>(next_ele);
    next_native->comp_type() =
        string_from_qking_to_string(next_ele->get_js_type());
    RAX_ASSERT(next_native->comp_type() == "text");
    // prepare value in attr
    next_native->MountChildrenForText();

    const auto &it = next_native->get_attrs().find("value");
    if (it != next_native->get_attrs().end()) {
      // found value
      set_attr("value", it->second);
      render_bridge::UpdateAttr(
          native_node(), new std::vector<std::pair<std::string, std::string>>{
                             {"value", it->second}});
    }
    return true;
  } else {
    return false;
  }
}

void RaxNativeElement::UpdateChildren(RaxNativeElement *next_ele) {
  if (UpdateComponentChildrenOfText(next_ele, 0)) {
    return;
  }
  UpdateComponentChildren(next_ele, this, native_node(), 0);
}

void RaxNativeElement::DebugPrintComponent() {
  RaxElement::DebugPrintComponent();
  std::vector<std::string> event_keys;
  std::transform(
      comp_events_.begin(), comp_events_.end(), std::back_inserter(event_keys),
      [](const std::map<std::string, std::unique_ptr<QKValueRef>>::value_type
             &pair) { return pair.first; });

  RAX_LOGD(
      "RaxComponent(%u): Native <%s>\n"
      "\t\tevent[%u]: %s\n"
      "\t\tstyle[%u]: %s\n"
      "\t\tattr[%u]: %s",
      eid(), comp_type().c_str(), static_cast<uint32_t>(comp_events_.size()),
      Json(event_keys).dump().c_str(),
      static_cast<uint32_t>(comp_styles_.size()),
      Json(comp_styles_).dump().c_str(),
      static_cast<uint32_t>(comp_attrs_.size()),
      Json(comp_attrs_).dump().c_str());
}

Json::object RaxNativeElement::DebugCollectComponent() {
  Json::object obj = RaxElement::DebugCollectComponent();

  std::vector<std::string> event_keys;
  std::transform(
      comp_events_.begin(), comp_events_.end(), std::back_inserter(event_keys),
      [](const std::map<std::string, std::unique_ptr<QKValueRef>>::value_type
             &pair) { return pair.first; });

  obj["tag"] = comp_type();

  obj["style"] = comp_styles_;
  obj["attr"] = comp_attrs_;
  obj["events"] = event_keys;

  // child
  Json::array child_array;
  for (auto &it : comp_children()) {
    child_array.push_back(it->DebugCollectComponent());
  }
  if (!child_array.empty()) {
    obj["children"] = child_array;
  }
  return obj;
}

native_node_ptr RaxNativeElement::get_first_render_node() {
  RAX_ASSERT(comp_native_node_);
  return comp_native_node_;
}
native_node_ptr RaxNativeElement::get_last_render_node() {
  RAX_ASSERT(comp_native_node_);
  return comp_native_node_;
}
RAX_NAME_SPACE_END
