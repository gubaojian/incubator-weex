//
// Created by Xu Jiacheng on 2018/12/25.
//

#include "rax_builtin_env.h"
#include "rax_class_element.h"
#include "rax_core_util.h"
#include "rax_element_factory.h"
#include "rax_render_bridge.h"
#include "rax_root_element.h"
#include "rax_update.h"

RAX_NAME_SPACE_BEGIN

static qking_value_t CreateElement(const qking_value_t function_obj,
                                   const qking_value_t this_val,
                                   const qking_value_t args_p[],
                                   const qking_length_t args_count) {
  //  RAX_LOGD("Builtin: CreateElement()");

  COMMON_START();

  qking_value_t key = qking_create_null();
  qking_value_t ref = qking_create_null();
  qking_value_t props = qking_create_object();
  qking_value_t children_array = qking_create_undefined();

  COMMON_RESOURCE();

  THROW_JS_ERR_IF(args_count < 2, "CreateElement: args less than 2");

  qking_value_t type = args_p[0];
  THROW_JS_ERR_IF(qking_value_is_null_or_undefined(type),
                  "CreateElement: type should not be null or undefined");

  qking_value_t config = args_p[1];

  // props, ref and key
  if (!qking_value_is_null_or_undefined(config)) {
    THROW_JS_ERR_IF(!qking_value_is_object(config),
                    "CreateElement: config is not a object");

    key = qking_get_property_by_name_lit(config, QKING_LIT_MAGIC_STRING_EX_KEY);
    CHECK_RESOURCE(key);
    if (qking_value_is_undefined(key)) {
      key = qking_create_null();
    } else {
      qking_value_t tmp = key;
      key = qking_value_to_string(key);
      qking_release_value(tmp);
      CHECK_RESOURCE(key);
    }

    ref = qking_get_property_by_name_lit(config, QKING_LIT_MAGIC_STRING_EX_REF);
    CHECK_RESOURCE(ref);
    if (qking_value_is_undefined(ref)) {
      ref = qking_create_null();
    }

    // props = config filter out key and ref.
    struct Passer {
      qking_value_t update_prop;
      qking_value_t key_string;
      qking_value_t ref_string;
    } tmp_pass{props, qking_create_string_lit(QKING_LIT_MAGIC_STRING_EX_KEY),
               qking_create_string_lit(QKING_LIT_MAGIC_STRING_EX_REF)};

    qking_foreach_object_property(
        config,
        [](const qking_value_t property_name,
           const qking_value_t property_value, void *user_data_p) {
          Passer *passer = (Passer *)user_data_p;
          qking_value_t update_prop = passer->update_prop;
          // ignore err if set failed
          if (qking_value_strict_equal(passer->key_string, property_name) ||
              qking_value_strict_equal(passer->ref_string, property_name)) {
            // skip ref or key
            return true;
          }
          qking_release_value(
              qking_set_property(update_prop, property_name, property_value));
          return true;
        },
        &tmp_pass);

    qking_release_value(tmp_pass.key_string);
    qking_release_value(tmp_pass.ref_string);
  }

  // children flatten
  children_array = rax_flatten_children(args_p + 2, args_count - 2);
  if (!qking_value_is_undefined(children_array)) {
    // has child.
    qking_release_value(qking_set_property_by_name_lit(
        props, QKING_LIT_MAGIC_STRING_EX_CHILDREN, children_array));
  } else {
    // using exist props.children.
    children_array = qking_get_property_by_name_lit(
        props, QKING_LIT_MAGIC_STRING_EX_CHILDREN);
  }
  // resolve default props
  // todo not supported yet.

  // flatten style
  {
    qking_value_t style_prop =
        qking_create_string_lit(QKING_LIT_MAGIC_STRING_EX_STYLE);
    qking_value_t old_style = qking_get_property(props, style_prop);
    CHECK_TMP_VAR_RELEASE(old_style, style_prop);
    qking_value_t new_style = rax_flatten_style(old_style);
    qking_release_value(old_style);
    CHECK_TMP_VAR_RELEASE(new_style, style_prop);
    qking_release_value(qking_set_property(props, style_prop, new_style));
    qking_release_value(new_style);
    qking_release_value(style_prop);
  }

  const std::string &page_id = rax_get_current_page_name();
  RAX_ASSERT(!page_id.empty());
  RaxElementFactory *factory = RaxElementFactory::GetFactory(page_id);
  RAX_ASSERT(factory);
  RaxElement *native_ele =
      factory->CreateElement(type, props, children_array, key, ref);
  THROW_JS_ERR_IF(!native_ele, "CreateElement: Invalid element type");

  qking_value_t final_value = factory->MakeJSValue(native_ele);
  func_ret = final_value;

  COMMON_FINALIZE();

  qking_release_value(key);
  qking_release_value(ref);
  qking_release_value(props);
  qking_release_value(children_array);

  COMMON_END();
}

static qking_value_t Render(const qking_value_t function_obj,
                            const qking_value_t this_val,
                            const qking_value_t args_p[],
                            const qking_length_t args_count) {
  RAX_LOGD("Builtin: Render()");

  COMMON_START();

  COMMON_RESOURCE();

  THROW_JS_ERR_IF(args_count != 1, "Render: Invalid argument count");

  const std::string &page_id = rax_get_current_page_name();
  RAX_ASSERT(!page_id.empty());
  RaxElementFactory *factory = RaxElementFactory::GetFactory(page_id);
  RAX_ASSERT(factory);

  THROW_JS_ERR_IF(factory->root(), "Render: Render twice, already has a root");

  RaxEleNativeType *element = nullptr;
  // todo. must be a Element of Class, Native, Function. but Rax also support
  // fragment, which we don't
  THROW_JS_ERR_IF(
      !qking_get_object_native_pointer(args_p[0], (void **)&element, nullptr),
      "Render: arg[0] is not a element");
  RaxElement *extract_element = factory->ExtractElement(element);
  RAX_MAKESURE(extract_element);

  RaxRootElement *root = factory->CreateRootElement();
  root->child() = extract_element;

  factory->root() = root;
  native_node_ptr render_root = render_bridge::NativeNodeCreateRootNode();

  root->MountComponent(render_root, nullptr, nullptr);

  render_bridge::SetRootNode(render_root);

  // todo. Rax returns native node for NativeElement and array for
  // FragmentElement.
  if (extract_element->type() == RaxElementType::kClass) {
    func_ret = qking_acquire_value(
        ((RaxClassElement *)extract_element)->get_class_instance());
  } else if (extract_element->type() == RaxElementType::kFunction) {
    func_ret = qking_create_null();
  }

  factory->CleanUpUnmountedComponent();

  COMMON_FINALIZE();

  COMMON_END();
}

static qking_value_t SetState(const qking_value_t function_obj,
                              const qking_value_t this_val,
                              const qking_value_t args_p[],
                              const qking_length_t args_count) {
  RAX_LOGD("Builtin: SetState()");

  COMMON_START();

  COMMON_RESOURCE();

  // component_class_instance,  new_partial_state;
  THROW_JS_ERR_IF(args_count != 2, "SetState: args num not 2");

  const std::string &page_id = rax_get_current_page_name();
  RAX_ASSERT(!page_id.empty());
  RaxElementFactory *factory = RaxElementFactory::GetFactory(page_id);
  RAX_ASSERT(factory);

  qking_value_t component_class_instance = args_p[0];
  qking_value_t partial_state = args_p[1];
  RaxElement *ele_to_update = nullptr;
  {
    qking_value_t native_ele_obj = qking_get_property_by_name_lit(
        component_class_instance, QKING_LIT_MAGIC_STRING_EX_UD_INTERNAL);
    CHECK_TMP_VAR(native_ele_obj);
    RaxEleNativeType *eleNativeType = nullptr;
    THROW_JS_ERR_IF(
        !qking_get_object_native_pointer(native_ele_obj,
                                         (void **)&eleNativeType, nullptr),
        "SetState: setState on a component with invalid '_internal' stub");
    ele_to_update = factory->ExtractElement(native_ele_obj);
    qking_release_value(native_ele_obj);
  }

  THROW_JS_ERR_IF(!ele_to_update, "SetState: RaxElement already released");
  THROW_JS_ERR_IF(ele_to_update->type() != RaxElementType::kClass,
                  "SetState: SetState on a none-class Component");
  RaxClassElement *class_element = (RaxClassElement *)ele_to_update;

  class_element->state_queue_push(partial_state);

  if (class_element->comp_mounted() && !class_element->comp_pending_state() &&
      !class_element->state_queue_empty()) {
    // root ele has only 1 child, insert_start is 0;
    g_by_state = 1;
    class_element->UpdateComponent(class_element, class_element, 0);
    RAX_ASSERT(g_by_state == 0);

    factory->CleanUpUnmountedComponent();
  } else {
    RAX_LOGD(
        "Builtin: SetState UpdateComponent skip: mounted=%s, "
        "comp_pending_state=%s, state_queue_empty=%s",
        class_element->comp_mounted() ? "true" : "false",
        class_element->comp_pending_state() ? "true" : "false",
        class_element->state_queue_empty() ? "true" : "false");
  }

  COMMON_FINALIZE();

  COMMON_END();
}

static qking_value_t RegisterDocumentEventHandler(
    const qking_value_t function_obj, const qking_value_t this_val,
    const qking_value_t args_p[], const qking_length_t args_count) {
  RAX_LOGD("Builtin: RegisterGlobalEventHandler()");

  COMMON_START();

  COMMON_RESOURCE();

  THROW_JS_ERR_IF(args_count != 1,
                  "RegisterGlobalEventHandler: Invalid argument count");

  const std::string &page_id = rax_get_current_page_name();
  RAX_ASSERT(!page_id.empty());
  RaxElementFactory *factory = RaxElementFactory::GetFactory(page_id);
  RAX_ASSERT(factory);

  THROW_JS_ERR_IF(
      !qking_value_is_function(args_p[0]),
      "RegisterDocumentEventHandler: argument needs to be a function");

  factory->set_document_event_handler(args_p[0]);

  COMMON_FINALIZE();

  COMMON_END();
}

void RegisterRaxBuiltin() {
  qking_external_handler_register_global("__createElement", CreateElement);
  qking_external_handler_register_global("__render", Render);
  qking_external_handler_register_global("__setState", SetState);
  qking_external_handler_register_global("__registerDocumentEventHandler",
                                         RegisterDocumentEventHandler);
}

RAX_NAME_SPACE_END
