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
#include "core/common/view_utils.h"
#include "core/network/http_module.h"
#include "core/manager/weex_core_manager.h"
#include "base/make_copyable.h"
#include "core/data_render/qking/vnode_qking_convert.h"
#include "core/data_render/vnode/vnode_render_context.h"
#include "core/data_render/qking/vnode_qking_env.h"
#include "core/data_render/vnode/vnode_render_manager.h"
#include "core/data_render/vnode/vcomponent_lifecycle_listener.h"
#include "core/data_render/vnode/vnode_on_event_listener.h"
#include "base/log_defines.h"
#include "qking/rax/rax_builtin_env.h"
#include "base/common_error.h"

namespace weex {
namespace core {
namespace data_render {

#ifdef QKING_NDEBUG
static void qking_handler_fatal_error(int code) {
    throw quick::king::FatalError(code);
}
#endif
    
CallbackManager::~CallbackManager() {
    for (auto iter = callback_map_.begin(); iter != callback_map_.end(); iter++) {
        qking_release_value(iter->second);
    }
}
    
uint32_t CallbackManager::AddCallback(qking_value_t callback) {
    callback_id_++;
    callback_map_[callback_id_] = callback;
    return callback_id_;
}

bool CallbackManager::Call(uint32_t id, const std::string &data, bool keep_alive) {
    bool is_error = false;
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
        qking_value_t completion_value = qking_create_undefined();
        if (data.length() > 0) {
            qking_value_t result = qking_json_parse((const qking_char_t *)data.c_str(), (qking_size_t)data.length());
            try {
                completion_value = qking_call_function(func, qking_create_undefined(), &result, 1);
            }
            catch (std::exception &e) {
                LOGE("[exception]:=>qking call back error: %s", e.what());
                is_error = true;
            }
            qking_release_value(result);
        }
        else {
            try {
                completion_value = qking_call_function(func, qking_create_undefined(), nullptr, 0);
            }
            catch (std::exception &e) {
                LOGE("[exception]:=>qking call back error: %s", e.what());
                is_error = true;
            }
        }
        if (!is_error) {
            qking_run_all_enqueued_jobs();
        }
        if (!keep_alive) {
            qking_release_value(func);
            callback_map_.erase(iter);
        }
        qking_release_value(completion_value);
        
    } while (0);
    
    return !is_error;
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

  return result;//only the first function will return now.
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
    
static qking_value_t Print(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
    std::string log_result;
    for (int i = 0; i < args_count; ++i) {
      log_result.append(string_from_qking_to_string(args_p[i]));
      log_result.append(" ");
    }
    WeexCoreManager::Instance()
      ->getPlatformBridge()
      ->platform_side()
      ->NativeLog(log_result.c_str());
    return qking_print_log("QK_JSLog: ",function_obj,this_val,args_p,args_count);
}

static qking_value_t EmptyFunc(const qking_value_t function_obj, const qking_value_t this_val, const qking_value_t args_p[], const qking_length_t args_count) {
  return qking_create_undefined();
}
    
void VNodeQkingEnv::Register()
{
    // release must register fatal error, debug abort to discover the problem
#ifdef QKING_NDEBUG
    qking_register_handler_fatal_error(qking_handler_fatal_error);
#endif
    rax::RegisterRaxBuiltin();
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
