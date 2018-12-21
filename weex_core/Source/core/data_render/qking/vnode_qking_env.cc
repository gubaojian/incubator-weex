/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <sstream>
#include "core/network/http_module.h"
#include "core/manager/weex_core_manager.h"
#include "base/make_copyable.h"
#include "core/data_render/qking/vnode_qking_convert.h"
#include "core/data_render/vnode/vnode_render_context.h"
#include "core/data_render/qking/vnode_qking_env.h"
#include "core/data_render/vnode/vnode_render_manager.h"
#include "core/data_render/vnode/vcomponent_lifecycle_listener.h"
#include "core/data_render/vnode/vnode_on_event_listener.h"
#include "base/LogDefines.h"
#include "base/ViewUtils.h"

namespace weex {
namespace core {
namespace data_render {
    
uint32_t CallbackManager::AddCallback(qking_value_t callback) {
    callback_id_++;
    callback_map_[callback_id_] = callback;
    return callback_id_;
}

void CallbackManager::Call(uint32_t id, const std::string &data, bool keep_alive) {
    do {
        auto iter = callback_map_.find(id);
        if (iter == callback_map_.end()) {
            break;
        }
        qking_value_t func = iter->second;
        if (!qking_value_is_function(func)) {
            LOGE("[data-render] InvokeCallback: not callable");
            break;
        }
        //todo add args;
        if (data.length() > 0) {
            qking_value_t result = qking_json_parse((const qking_char_t *)data.c_str(), (qking_size_t)data.length());
            qking_call_function(func, qking_create_undefined(), &result, 1);
            qking_release_value(result);
        }
        else {
            qking_call_function(func, qking_create_undefined(), nullptr, 0);
        }
        if (!keep_alive) {
            qking_release_value(func);
            callback_map_.erase(iter);
        }
        
    } while (0);
}
    
static qking_value_t CallNativeModule(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
  qking_value_t result = qking_create_undefined();
  do {
        if (args_count < 1) {
            break;
        }
        if (!qking_value_is_object(args_p[0])) {
            break;
        }
        std::string module = string_from_qking_get_property_by_name(args_p[0], "module");
        if (module.empty()) {
            break;
        }
        std::string method = string_from_qking_get_property_by_name(args_p[0], "method");
        if (method.empty()) {
            break;
        }
        qking_value_t args_var = qking_get_property_by_name(args_p[0], "args");
        if (!qking_value_is_object(args_var)) {
            qking_release_value(args_var);
            break;
        }
        qking_value_t length_var = qking_get_property_by_name(args_var, "length");
        if (!qking_value_is_number(length_var)) {
            qking_release_value(length_var);
            break;
        }
        int argc = qking_get_number_value(length_var);
        qking_release_value(length_var);
        std::string args;
        CallbackManager *callback_manager = weex::core::data_render::VNodeRenderManager::GetInstance()->GetCallbackManager(qking_get_current_executor());
        if (callback_manager == nullptr) {
            break;
        }
        if (argc > 0) {
            qking_value_t args_array = qking_create_array(argc);
            for (int i = 0; i < argc; i++) {
                qking_value_t var = qking_get_property_by_index(args_var, i);
                bool is_function = qking_value_is_function(var);
                if (is_function) {
                  uint32_t func_callback = callback_manager->AddCallback(var);
                  var = qking_create_string((const qking_char_t *)(WeexCore::to_string(
                      func_callback).c_str()));
                  if (qking_value_is_undefined(result)) {
                    result = qking_acquire_value(var);//make a copy.
                  }
                }
                qking_set_property_by_index(args_array, i, var);
                qking_release_value(var);
            }
            args = string_from_qking_json_stringify(args_array);
            qking_release_value(args_array);
        }
        qking_release_value(args_var);
        weex::core::data_render::VNodeRenderManager::GetInstance()->CallNativeModule(qking_get_current_executor(), module.c_str(), method.c_str(), argc > 0 ? args.c_str() : "", argc);
        
    } while(0);

  return result;
}
    
static qking_value_t RequireModule(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    
    qking_value_t result = qking_create_undefined();
    do {
        if (!args_count) {
            break;
        }
        if (!qking_value_is_string(args_p[0])) {
            break;
        }
        std::string module_name = string_from_qking_string_value(args_p[0]);
        std::string module_info;
        if (!weex::core::data_render::VNodeRenderManager::GetInstance()->RequireModule(module_name, module_info)) {
            break;
        }
        result = qking_json_parse((const qking_char_t *)module_info.c_str(), (qking_size_t)module_info.length());
        
    } while (0);
    
    return result;
}

static qking_value_t UpdateElement(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    
    qking_value_t result;
    do {
        if (args_count < 2) {
            result = qking_create_error(QKING_ERROR_TYPE, "UpdateElement needs >= 2 args");
            break;
        }
        VNode *vn_prev = NULL;
        if (!qking_get_object_native_pointer(args_p[0], (void **)&vn_prev, NULL) || !vn_prev) {
            result = qking_create_error(QKING_ERROR_TYPE, "UpdateElement needs prev vnode");
            break;
        }
        VNode *vn_next = NULL;
        if (!qking_get_object_native_pointer(args_p[1], (void **)&vn_next, NULL) || !vn_next) {
            result = qking_create_error(QKING_ERROR_TYPE, "UpdateElement needs next vnode");
            break;
        }
        VNodeRenderManager::GetInstance()->PatchVNode(qking_get_current_executor(), vn_prev, vn_next);
        result = qking_create_undefined();
        
    } while (0);
    
    return result;
}

    
// createElement("tag_name", "id", ref);
static qking_value_t CreateElement(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    qking_value_t result;
    do {
        if (!qking_value_is_string(args_p[0])) {
            result = qking_create_error(QKING_ERROR_TYPE, "CreateElement args 0 type error");
            break;
        }
        std::string tag_name = string_from_qking_string_value(args_p[0]);
        std::string node_id, ref;
        if (qking_value_is_string(args_p[1])) {
            node_id = string_from_qking_string_value(args_p[1]);
        }
        else if (qking_value_is_number(args_p[1])) {
            std::ostringstream os;
            os << static_cast<int>(qking_get_number_value(args_p[1]));
            node_id = "vn_" + os.str();
        }
        else {
            result = qking_create_error(QKING_ERROR_TYPE, "createElement args 1 type error");
            break;
        }
        if (args_count > 2 && qking_value_is_string(args_p[2])) {
            ref = string_from_qking_string_value(args_p[2]);
        }
        LOGD("[VM][VNode][CreateElement]: %s  %s\n", node_id.c_str(), tag_name.c_str());
        VNodeRenderContext *render_context = (VNodeRenderContext *)qking_get_current_external_context();
        VNode *node = NULL;
        if (tag_name == "root") {
            node = new VNode("div", "vn_r", "vn_r");
            render_context->set_root(node);
        }
        else {
            node = new VNode(tag_name, node_id, ref);
            if (render_context->root() == nullptr) {
                render_context->set_root(node);
            }
        }
        result = qking_create_object_native_pointer(node, nullptr);

    } while (0);

    return result;
}
    
static void AppendChildInternal(VNode* parent, VNode* children) {
    if (parent && children) {
        parent->AddChild(children);
    }
}

// appendChild(parent, node);
static qking_value_t AppendChild(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    
    qking_value_t result;
    do {
        VNode *parent = NULL;
        if (!qking_get_object_native_pointer(args_p[0], (void **)&parent, NULL)) {
            result = qking_create_error(QKING_ERROR_TYPE, "AppendChild needs parent vnode");
            break;
        }
        if (qking_value_is_string(args_p[1]) && parent->tag_name() != "text") {
            result = qking_create_undefined();//skip
            break;
        }
        else if (qking_value_is_null(args_p[1]) || qking_value_is_undefined(args_p[1])) {
            result = qking_create_undefined();
            break;
        }
        else if (qking_value_is_array(args_p[1])) {
            uint32_t array_length = qking_get_array_length(args_p[1]);
            for (int i = 0; i < array_length; i++) {
                qking_value_t item = qking_get_property_by_index(args_p[1], i);
                if (qking_value_is_undefined(item) || qking_value_is_null(item)) {
                    continue;
                }
                VNode *children = NULL;
                if (!qking_get_object_native_pointer(item, (void **)&children, NULL)) {
                    qking_release_value(item);
                    result = qking_create_error(QKING_ERROR_TYPE, "AppendChild type vnode");
                    return result;
                }
                AppendChildInternal(parent, children);
                qking_release_value(item);
            }
        }
        else if (qking_value_is_string(args_p[1]) && parent) {
            std::string text = string_from_qking_string_value(args_p[1]);
            parent->SetAttribute("value", text);
            LOGD("[Render]=>SetAttribute:%s\n", text.c_str());
        }
        else {
            VNode *children = NULL;
            if (!qking_get_object_native_pointer(args_p[1], (void **)&children, NULL)) {
                result = qking_create_error(QKING_ERROR_TYPE, "AppendChild type vnode");
                break;
            }
            AppendChildInternal(parent, children);
        }
        result = qking_create_undefined();
        
    } while (0);
    
    return result;
}

// setAttr(node, "value");
static qking_value_t SetAttr(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    qking_value_t result;
    VNode *node = NULL;
    do {
        if (!qking_get_object_native_pointer(args_p[0], (void **)&node, NULL)) {
            result = qking_create_error(QKING_ERROR_TYPE, "SetAttr type vnode");
            break;
        }
        std::string key = string_from_qking_string_value(args_p[1]);
        std::string attr;
        if (!qking_value_is_string(args_p[2])) {
            qking_value_t str_conv = qking_value_to_string(args_p[2]);
            attr = string_from_qking_string_value(str_conv);
            qking_release_value(str_conv);
        }
        else {
            attr = string_from_qking_string_value(args_p[2]);
        }
        node->SetAttribute(key, attr);
        result = qking_create_undefined();
        
    } while (0);
    
    return result;
}

// setProps(node, "value");
static qking_value_t SetProps(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    qking_value_t result = qking_create_undefined();
    do {
        VNode *node = NULL;
        if (!qking_get_object_native_pointer(args_p[0], (void **)&node, NULL)) {
            result = qking_create_error(QKING_ERROR_TYPE, "SetProps type error vnode");
            break;
        }
        if (!qking_value_is_object(args_p[1])) {
            break;
        }
        LOGD("[Render]=>style: %s\n", qking_value_print(args_p[1]).c_str());
        qking_value_t keys = qking_get_object_keys(args_p[1]);
        if (!qking_value_is_array(keys)) {
            break;
        }
        uint32_t length = qking_get_array_length(keys);
         LOGD("[Render]=>style length:%u\n", length);

      for (int i = 0; i < length; i++) {
            qking_value_t key = qking_get_property_by_index(keys, i);
            qking_value_t item = qking_get_property(args_p[1], key);
            std::string keystr = string_from_qking_string_value(key);
            qking_release_value(key);
            if (keystr == "style") {
                if (!qking_value_is_object(item)) {
                    qking_release_value(item);
                    continue;
                }
                qking_value_t style_keys = qking_get_object_keys(item);
                if (!qking_value_is_array(style_keys)) {
                    qking_release_value(style_keys);
                    qking_release_value(item);
                    continue;
                }
                uint32_t style_length = qking_get_array_length(style_keys);
                for (int j = 0; j < style_length; j++) {
                    qking_value_t style_key = qking_get_property_by_index(style_keys, j);
                    std::string style_keystr = string_from_qking_string_value(style_key);
                    qking_value_t style_item = qking_get_property(item, style_key);
                    std::string style_str;
                    if (!qking_value_is_string(style_item)) {
                        qking_value_t str_conv = qking_value_to_string(style_item);
                        style_str = string_from_qking_string_value(str_conv);
                        qking_release_value(str_conv);
                    }
                    else {
                        style_str = string_from_qking_string_value(style_item);
                    }
                    qking_release_value(style_key);
                    qking_release_value(style_item);
                    LOGD("[Render]=>SetStyle:%s key:%s var:%s\n", node->node_id().c_str(), style_keystr.c_str(), style_str.c_str());
                    node->SetStyle(style_keystr, style_str);
                }
                qking_release_value(item);
                qking_release_value(style_keys);
            }
            else if (qking_value_is_function(item)) {
                std::string::size_type pos = keystr.find("on");
                if (pos != 0) {
                    result = qking_create_error(QKING_ERROR_TYPE, "AddEvent isn't a function");
                    break;
                }
                std::string event = keystr.substr(pos + 2);
                transform(event.begin(), event.end(), event.begin(), ::tolower);
                // can't deref item for addEvent
                node->AddEvent(event, item);
                LOGD("[Render]=>SetEvent:%s key:%s var:%u\n", node->node_id().c_str(), keystr.c_str(),item);
            }
            else {
                std::string props;
                if (!qking_value_is_string(item)) {
                    qking_value_t str_conv = qking_value_to_string(item);
                    props = string_from_qking_string_value(str_conv);
                    qking_release_value(str_conv);
                }
                else {
                    props = string_from_qking_string_value(item);
                }
                qking_release_value(item);
                LOGD("[Render]=>SetAttribute:%s key:%s var:%s\n", node->node_id().c_str(), keystr.c_str(), props.c_str());
                node->SetAttribute(keystr, props);
            }
        }
        qking_release_value(keys);
        
    } while (0);
    
    return result;
}

// setStyle(node, key, value);
inline static qking_value_t SetStyleInternal(VNode* node, qking_value_t key, qking_value_t value) {
    qking_value_t result = qking_create_undefined();
    do {
        if (node == nullptr || !qking_value_is_string(key)) {
            //skip instead of raise err
            break;
        }
        std::string keystr = string_from_qking_string_value(key);
        std::string var;
        if (!qking_value_is_string(value)) {
            qking_value_t str_conv = qking_value_to_string(value);
            var = string_from_qking_string_value(str_conv);
            qking_release_value(str_conv);
        }
        else {
            var = string_from_qking_string_value(value);
        }
        node->SetStyle(keystr, var);
        
    } while (0);
    
    return result;
}

// setStyle(node, style);
inline static qking_value_t SetStyleInternal(VNode* node, qking_value_t style) {
    qking_value_t result = qking_create_undefined();
    do {
        if (node == nullptr || !qking_value_is_object(style)) {
          //skip instead of raise err
            break;
        }
        qking_value_t keys = qking_get_object_keys(style);
        if (!qking_value_is_array(keys)) {
            break;
        }
        uint32_t length = qking_get_array_length(keys);
        for (int i = 0; i < length; i++) {
            qking_value_t key = qking_get_property_by_index(keys, i);
            std::string keystr = string_from_qking_string_value(key);
            qking_value_t item = qking_get_property(style, key);
            std::string style_str;
            if (!qking_value_is_string(item)) {
                qking_value_t str_conv = qking_value_to_string(item);
                style_str = string_from_qking_string_value(str_conv);
                qking_release_value(str_conv);
            }
            else {
                style_str = string_from_qking_string_value(item);
            }
            node->SetStyle(keystr, style_str);
            qking_release_value(key);
            qking_release_value(item);
        }
        qking_release_value(keys);
        
    } while (0);
    
    return result;
}

// setStyle(node, key, value);
// or
// setStyle(node, style);
static qking_value_t SetStyle(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    qking_value_t result = qking_create_undefined();
    do {
        VNode *node = NULL;
        if (!qking_get_object_native_pointer(args_p[0], (void **)&node, NULL)) {
            result = qking_create_error(QKING_ERROR_TYPE, "SetStyle type error vnode");
            break;
        }
        if (args_count == 3) {
            result = SetStyleInternal(node, args_p[1], args_p[2]);
        }
        else {
            result = SetStyleInternal(node, args_p[1]);
        }
        
    } while (0);
    
    return result;
}

static qking_value_t Print(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    return qking_print_log("JSLog: ",function_obj,this_val,args_p,args_count);
}

static qking_value_t EmptyFunc(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
  return qking_create_undefined();
}

void VNodeQkingEnv::Register()
{
    qking_external_handler_register_global("createElement", CreateElement);
    qking_external_handler_register_global("updateElement", UpdateElement);
    qking_external_handler_register_global("appendChild", AppendChild);
    qking_external_handler_register_global("setAttr", SetAttr);
    qking_external_handler_register_global("setProps", SetProps);
    qking_external_handler_register_global("setStyle", SetStyle);
    qking_external_handler_register_global("__callNativeModule", CallNativeModule);
    qking_external_handler_register_global("__requireModule", RequireModule);
    qking_external_handler_register_global("__print", Print);

    qking_external_handler_register_global("__isRegisteredModule", EmptyFunc);//todo
    qking_external_handler_register_global("__isRegisteredComponent", EmptyFunc);
}
    
void VNodeQkingEnv::RegisterVariable(const std::string &init_data_str) {
    do {
        qking_value_t result = qking_json_parse((const qking_char_t *)init_data_str.c_str(), (qking_length_t)init_data_str.length());
        if (qking_value_is_error(result)) {
            qking_release_value(result);
            break;
        }
        qiking_external_variable_register_global("_init_data", result);
        qiking_external_variable_register_global("__weex_data__", result);
        qking_release_value(result);
        
    } while (0);
}

void VNodeQkingEnv::RegisterVariable(const char *name_p, const std::string &jsonstr) {
    do {
        qking_value_t result = qking_json_parse((const qking_char_t *)jsonstr.c_str(), (qking_length_t)jsonstr.length());
        if (qking_value_is_error(result)) {
            break;
        }
        qiking_external_variable_register_global(name_p, result);
        qking_release_value(result);
        
    } while (0);
}

}  // namespace data_render
}  // namespace core
}  // namespace weex
