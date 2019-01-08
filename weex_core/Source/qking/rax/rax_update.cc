//
// Created by Xu Jiacheng on 2018/12/25.
//

#include "rax_update.h"
#include "rax_element_factory.h"
#include "rax_native_element.h"
#include "rax_render_bridge.h"

RAX_NAME_SPACE_BEGIN

uint32_t g_by_state = false;

bool ShouldUpdateComponent(RaxElement* prev, RaxElement* next) {
  // nullptr is not a valid input.
  RAX_ASSERT(prev && next);
  RaxElementType prev_type = prev->type();
  RaxElementType next_type = next->type();

  RAX_ASSERT(prev_type != RaxElementType::kRoot);
  RAX_ASSERT(next_type != RaxElementType::kRoot);

  if (prev_type != next_type) {
    return false;
  }

  if (prev_type == RaxElementType::kText ||
      prev_type == RaxElementType::kEmpty ||
      prev_type == RaxElementType::kFragement) {
    return true;
  }

  if (!qking_value_strict_equal(prev->get_js_type(), next->get_js_type())) {
    return false;
  }
  if (!qking_value_strict_equal(prev->get_js_key(), next->get_js_key())) {
    return false;
  }
  (void)0;
  return true;  // same type
}

using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

static vector<pair<string, string>>* CompareMap(
    const map<string, string>& oldMap, const map<string, string>& newMap) {
  auto p_vec = new vector<pair<string, string>>();
  for (auto it = newMap.cbegin(); it != newMap.cend(); it++) {
    auto pos = oldMap.find(it->first);
    if (pos == oldMap.end() || pos->second != it->second) {
      // key not exist, or value not same
      p_vec->push_back({it->first, it->second});
    }
  }

  for (auto it = oldMap.cbegin(); it != oldMap.cend(); it++) {
    auto pos = newMap.find(it->first);
    if (pos == newMap.end()) {
      // key not exist, remove //todo check if this is correct
      p_vec->push_back({it->first, ""});
    }
  }
  return p_vec;
};

static void CompareAndApplyEvents(RaxNativeElement* old_node,
                                  RaxNativeElement* new_node) {
  map<string, std::unique_ptr<QKValueRef>>& old_events = old_node->get_events();
  map<string, std::unique_ptr<QKValueRef>>& new_events = new_node->get_events();
  set<string> remove_events;
  set<string> add_events;

  for (auto it = old_events.cbegin(); it != old_events.cend(); it++) {
    auto pos = new_events.find(it->first);
    if (pos == new_events.end()) {
      remove_events.insert(it->first);
    }
  }
  for (auto it = new_events.cbegin(); it != new_events.cend(); it++) {
    auto pos = old_events.find(it->first);
    if (pos == old_events.end()) {
      add_events.insert(it->first);
    }
  }
  for (auto it = remove_events.cbegin(); it != remove_events.cend(); it++) {
    render_bridge::RemoveEvent(old_node->native_node(), *it);
  }
  for (auto it = add_events.cbegin(); it != add_events.cend(); it++) {
    render_bridge::AddEvent(old_node->native_node(), *it);
  }
  old_events.clear();
  old_events = std::move(new_events);
  RAX_ASSERT(new_events.empty());
}

void UpdateNodeProperties(RaxNativeElement* old_node,
                          RaxNativeElement* new_node) {
  auto p_vec = CompareMap(old_node->get_attrs(), new_node->get_attrs());
  if (p_vec->size() > 0) {
    for (auto& it : *p_vec) {
      old_node->set_attr(it.first, it.second);
    }
    render_bridge::UpdateAttr(old_node->native_node(), p_vec);
  } else {
    delete p_vec;
  }
  // compare style, ptr will be delete by RenderPage
  p_vec = CompareMap(old_node->get_styles(), new_node->get_styles());
  if (p_vec->size()) {
    for (auto& it : *p_vec) {
      old_node->set_style(it.first, it.second);
    }
    render_bridge::UpdateStyle(old_node->native_node(), p_vec);
  } else {
    delete p_vec;
  }

  CompareAndApplyEvents(old_node, new_node);
}

void PrepareChildrenElement(RaxElement* next_ele) {
  qking_value_t child = next_ele->get_js_children();
  if (!qking_value_is_array(child)) {
    if (!qking_value_is_undefined(child)) {
      // only one.
      RaxElement* child_ele = next_ele->get_factory()->GetRawTypeElement(child);
      if (child_ele == nullptr) {
        throw rax_common_err("Native MountChildren: Not a valid child type");
      }
      next_ele->comp_children().push_back(child_ele);
    }
    // else { no children }
  } else {
    struct Passer {
      RaxElement* parent_this;
      bool error;
    } tmp{next_ele, false};
    qking_foreach_object_property_of(
        child,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void* user_data_p) -> bool {
          Passer* passer = (Passer*)user_data_p;
          RaxElement* parent_this = passer->parent_this;
          RaxElement* child_ele =
              parent_this->get_factory()->GetRawTypeElement(property_value);
          if (child_ele == nullptr) {
            passer->error = true;
            return false;
          }
          parent_this->comp_children().push_back(child_ele);
          return true;
        },
        &tmp, true, true, false);

    if (tmp.error) {
      throw rax_common_err("Native MountChildren: Not a valid child type");
    }
  }
}

std::string GetKeyNameOfElement(std::map<std::string, RaxElement*>& map,
                                RaxElement* element, uint32_t index) {
  qking_value_t key = element->get_js_key();
  std::string key_str;
  if (!qking_value_is_string(key)) {
    key_str = "." + to_string(index);
  } else {
    // has key.
    key_str = "$" + string_from_qking_string_value(key);
  }

  if (map.find(key_str) != map.end()) {
    RAX_LOGW("Has duplicate key: %s", key_str.c_str());
    key_str = "." + to_string(index);
  }
  RAX_ASSERT(map.find(key_str) == map.end());
  map[key_str] = element;
  return key_str;
}

static uint32_t GetInsertStartIndexOf(RaxElement* element, RaxElement* parent,
                                      native_node_ptr parent_node) {
  RAX_ASSERT(element);
  RAX_ASSERT(parent->type() == RaxElementType::kNative ||
             parent->type() == RaxElementType::kFragement);
  RAX_ASSERT(parent_node);

  const auto& comp_children = parent->comp_children();
  auto it = std::find(comp_children.begin(), comp_children.end(), element);
  RAX_ASSERT(it != comp_children.end());

  native_node_ptr first_node = element->get_first_render_node();
  RaxElement* ele_ptr = element;

  while (first_node == nullptr && ele_ptr != nullptr) {
    first_node = element->get_first_render_node();
    if (it == comp_children.begin()) {
      ele_ptr = nullptr;
    } else {
      it--;
      ele_ptr = *it;
      RAX_ASSERT(ele_ptr);
    }
  }

  if (first_node == nullptr) {
    return 0;  // front of child.
  } else {
    int32_t index_of =
        render_bridge::NativeNodeIndexOf(parent_node, first_node);
    RAX_ASSERT(index_of >= 0);
    return static_cast<uint32_t>(index_of);
  }
}

static uint32_t GetInsertEndIndexOf(RaxElement* element, RaxElement* parent,
                                    native_node_ptr parent_node) {
  RAX_ASSERT(element);
  RAX_ASSERT(parent->type() == RaxElementType::kNative ||
             parent->type() == RaxElementType::kFragement);
  RAX_ASSERT(parent_node);

  const auto& comp_children = parent->comp_children();
  auto it = std::find(comp_children.begin(), comp_children.end(), element);
  RAX_ASSERT(it != comp_children.end());

  native_node_ptr last_node = element->get_last_render_node();
  if (!last_node) {
    return GetInsertStartIndexOf(element, parent, parent_node);  // empty
  }
  int32_t index_of = render_bridge::NativeNodeIndexOf(parent_node, last_node);
  RAX_ASSERT(index_of >= 0);
  return static_cast<uint32_t>(index_of) + 1;
}

void UpdateComponentChildren(RaxElement* next_ele, RaxElement* current_ele,
                             native_node_ptr parent_node,
                             uint32_t insert_start_idx) {
  using std::map;
  using std::pair;
  using std::string;
  using std::vector;

  auto& comp_children = current_ele->comp_children();

  map<string, RaxElement*>
      old_children_by_name;  // index will be invalid after unmount.
  map<string, RaxElement*> new_children_by_name;

  vector<string> old_names;
  vector<bool> old_markers;
  vector<string> new_names;

  RAX_ASSERT(next_ele != current_ele);

  // prepare next_ele children;
  PrepareChildrenElement(next_ele);

  // old
  for (uint32_t i = 0; i < comp_children.size(); i++) {
    string key_str =
        GetKeyNameOfElement(old_children_by_name, comp_children[i], i);
    old_names.push_back(key_str);
    old_markers.push_back(false);
  }

  // new
  for (uint32_t i = 0; i < next_ele->comp_children().size(); i++) {
    string key_str = GetKeyNameOfElement(new_children_by_name,
                                         next_ele->comp_children()[i], i);
    new_names.push_back(key_str);
  }

  // update all that could be updated, or unmount those who can't
  for (uint32_t i = 0; i < new_names.size(); i++) {
    const std::string& new_name = new_names[i];

    const auto& in_old_it = old_children_by_name.find(new_name);
    if (in_old_it == old_children_by_name.end()) {
      // not exist. no op
    } else {
      // already exist.

      RaxElement* old_ele = in_old_it->second;
      RAX_ASSERT(old_ele);

      const auto& in_new_it = new_children_by_name.find(new_name);
      RAX_ASSERT(in_new_it != new_children_by_name.end());
      RaxElement* new_ele = in_new_it->second;

      uint32_t old_index = find_index_of(old_names, new_name);

      if (ShouldUpdateComponent(old_ele, new_ele)) {
        if (old_ele != new_ele) {
          old_ele->UpdateComponent(
              old_ele, new_ele,
              GetInsertStartIndexOf(old_ele, current_ele, parent_node));
        }
        old_markers[old_index] = true;
      } else {
        // unmount old.
        old_ele->UnmountComponent(false);
        comp_children.erase(comp_children.begin() + old_index);
        old_names.erase(old_names.begin() + old_index);
        old_markers.erase(old_markers.begin() + old_index);
        find_and_remove(old_children_by_name, new_name);
      }
    }
  }

  for (uint32_t i = 0; i < old_names.size();) {
    if (old_markers[i]) {
      i++;
    } else {
      std::string old_name = old_names[i];
      RaxElement* old_ele = comp_children[i];

      old_ele->UnmountComponent(false);
      comp_children.erase(comp_children.begin() + i);
      old_names.erase(old_names.begin() + i);
      old_markers.erase(old_markers.begin() + i);
      find_and_remove(old_children_by_name, old_name);
    }
  }

  // all old_markers must be true now.
  RAX_ASSERT(std::find(old_markers.begin(), old_markers.end(), false) ==
             old_markers.end());
  RAX_ASSERT(old_names.size() == old_markers.size());
  RAX_ASSERT(comp_children.size() == old_markers.size());

  // align to new children array. mount or move.
  uint32_t to_align_comp = 0;
  uint32_t index_in_render = insert_start_idx;

  while (to_align_comp < new_names.size()) {
    const string& name = new_names[to_align_comp];
    const auto& it = old_children_by_name.find(name);

    index_in_render = to_align_comp == 0 ? insert_start_idx
                                         : GetInsertEndIndexOf(
                                               comp_children[to_align_comp - 1],
                                               current_ele, parent_node);

    if (it == old_children_by_name.end()) {
      // not exist in old
      const auto& new_it = new_children_by_name.find(name);
      RAX_ASSERT(new_it != new_children_by_name.end());

      RaxElement* new_child_comp = new_it->second;
      new_child_comp->MountComponent(
          parent_node, current_ele,
          [&index_in_render](native_node_ptr parent, native_node_ptr child) {
            render_bridge::InsertNode(parent, child, index_in_render);
            index_in_render++;
          });
      comp_children.insert(comp_children.begin() + to_align_comp,
                           new_child_comp);
    } else {
      // exist in old.
      RaxElement* old_comp = it->second;

      uint32_t index_of_exist_ele = find_index_of(comp_children, old_comp);
      RAX_ASSERT(to_align_comp <= index_of_exist_ele);

      if (index_of_exist_ele == to_align_comp) {
        // already in place
      } else {
        // move to front

        uint32_t old_from =
            GetInsertStartIndexOf(old_comp, current_ele, parent_node);
        uint32_t old_to =
            GetInsertEndIndexOf(old_comp, current_ele, parent_node);

        RAX_ASSERT(old_from <= old_to);
        RAX_ASSERT(index_in_render <= old_from);

        // move node
        if (index_in_render != old_from) {
          for (uint32_t i = old_from; i < old_to; ++i) {
            render_bridge::MoveNode(
                parent_node,
                render_bridge::NativeNodeChildOfIndex(parent_node, i),
                index_in_render);
            index_in_render++;
          }
        }

        // move comp
        comp_children.erase(comp_children.begin() + index_of_exist_ele);
        comp_children.insert(comp_children.begin() + to_align_comp, old_comp);
      }
    }

    to_align_comp++;
  }

  RAX_ASSERT(comp_children.size() == new_names.size());
}

RAX_NAME_SPACE_END
