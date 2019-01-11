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

#include "core/data_render/vnode/vnode_render_manager.h"
#include <chrono>
#include <sstream>
#include "base/make_copyable.h"
#include "base/string_util.h"
#include "core/bridge/platform_bridge.h"
#include "core/data_render/common_error.h"
#include "core/data_render/exec_state.h"
#include "core/data_render/string_table.h"
#include "core/data_render/vnode/vnode.h"
#include "core/data_render/vnode/vnode_exec_env.h"
#include "core/manager/weex_core_manager.h"
#include "core/network/http_module.h"
#include "core/render/manager/render_manager.h"
#include "core/render/node/factory/render_creator.h"
#include "core/data_render/qking/vnode_qking_env.h"
#include "core/data_render/qking/vnode_qking_convert.h"
#include "qking/rax/vdom/rax_element_factory.h"
#include "qking/rax/vdom/rax_native_element.h"

#define VRENDER_LOG true

#if VRENDER_LOG
#include "base/log_defines.h"
#endif

namespace weex {
namespace core {
namespace data_render {

using std::map;
using std::pair;
using std::string;
using std::vector;
using WeexCore::RenderManager;

void PatchVNode(const string& page_id, VNode* old_node, VNode* new_node);

VNodeRenderManager* VNodeRenderManager::g_instance = nullptr;
VM* VNodeRenderManager::g_vm = nullptr;
// TODO establish linkages between page ref_id
int ref_id = 0;

WeexCore::RenderObject* ParseVNode2RenderObject(VNode* vnode,
                                                WeexCore::RenderObject* parent, bool isRoot,
                                                int index,
                                                const string& pageId) {
  if (!vnode) {
    return nullptr;
  }
  if (vnode->IsVirtualComponent()) {
    VComponent* component = static_cast<VComponent*>(vnode);
    if (component->root_vnode() == nullptr) {
      component->UpdateData();
    }
    return ParseVNode2RenderObject(component->root_vnode(), parent, isRoot,
                                   index, pageId);
  }
  std::string ref_str;
  if (isRoot) {
    ref_str = "_root";
  } else {
    ref_str = base::to_string(ref_id++);
  }

  WeexCore::RenderObject* render_object = static_cast<WeexCore::RenderObject*>(
      WeexCore::RenderCreator::GetInstance()->CreateRender(vnode->tag_name(),
                                                           ref_str));
  // Link RenderObject and VNode
  vnode->set_render_object_ref(std::move(ref_str));

  // style
  map<string, string>* style = vnode->styles();
  for (auto it = style->begin(); it != style->end(); it++) {
    render_object->AddStyle(it->first, it->second);
  }

  // attr
  map<string, string>* attr = vnode->attributes();
  for (auto it = attr->begin(); it != attr->end(); it++) {
    render_object->AddAttr(it->first, it->second);
  }

  // event
  std::map<std::string, void *> *events = vnode->events();
  for (auto iter = events->begin(); iter != events->end(); iter++) {
      render_object->events()->insert(iter->first);
  }
  std::map<std::string, qking_value_t> *qking_events = vnode->qking_events();
  for (auto iter = qking_events->begin(); iter != qking_events->end(); iter++) {
      render_object->events()->insert(iter->first);
  }
  auto event_params_map = vnode->event_params_map();
  for (auto it = event_params_map->begin(); it != event_params_map->end();
       it++) {
    render_object->events()->insert(it->first);
  }

  // child
  vector<VNode*>* children = vnode->child_list();
  for (int i = 0; i < children->size(); i++) {
    ParseVNode2RenderObject((*children)[i], render_object, false, i, pageId);
  }

  render_object->set_page_id(pageId);

  render_object->ApplyDefaultStyle();
  render_object->ApplyDefaultAttr();

  // parent
  if (parent != nullptr) {
    parent->AddRenderObject(index, render_object);
  }

  return render_object;
}

WeexCore::RenderObject *VNode2RenderObject(VNode *root, const string& page_id) {
  return ParseVNode2RenderObject(root, nullptr, true, 0, page_id);
}

void Patch(const string& page_id, VNode* old_root, VNode* new_root);

bool VNodeRenderManager::CreatePageInternal(const string& page_id,
                                            VNode* vNode) {
  auto node = vnode_trees_.find(page_id);
  if (node != vnode_trees_.end()) {
    delete node->second;
    node->second = nullptr;
  }
  vnode_trees_[page_id] = vNode;

  auto render_root = VNode2RenderObject(vNode, page_id);

  RenderManager::GetInstance()->CreatePage(page_id, render_root);
  RenderManager::GetInstance()->CreateFinish(page_id);
  return true;
}

bool VNodeRenderManager::RefreshPageInternal(const string& page_id,
                                             VNode* new_node) {
  auto node = vnode_trees_.find(page_id);
  if (node == vnode_trees_.end()) {
    return false;
  }

  auto oldNode = node->second;
  Patch(page_id, oldNode, new_node);
  node->second = new_node;
  delete oldNode;
  return true;
}

bool VNodeRenderManager::ClosePageInternal(const string& page_id) {
    RenderManager::GetInstance()->ClosePage(page_id);
    qking_executor_t executor = NULL;
    auto iter = executors_.find(page_id);
    if (iter != executors_.end()) {
        executor = iter->second;
    }
    if (executor) {
        qking_set_current_executor(executor);
        DestroyExecutor();
    }
    auto node = vnode_trees_.find(page_id);
    if (node != vnode_trees_.end()) {
      if (node->second) {
        delete node->second;
      }
      vnode_trees_.erase(node);
    }
    return true;
}

void VNodeRenderManager::InitVM() {
  if (g_vm == nullptr) {
    g_vm = new VM();
  }
}
    
CallbackManager *VNodeRenderManager::GetCallbackManager(qking_executor_t executor) {
    for (auto iter = executors_.begin(); iter != executors_.end(); iter++) {
        if (iter->second == executor) {
            const auto& page_id = iter->first;
            auto it = callback_managers_.find(page_id);
            if (it != callback_managers_.end()){
                return it->second.get();
            }
        }
    }
    return nullptr;
}

void VNodeRenderManager::CreatePage(const std::string &input, const std::string &page_id, const  std::string &options, const std::string &init_data, std::function<void(const char*)> exec_js) {
    std::string err = CreatePageWithContent(input, page_id, options, init_data, exec_js);
    if (!err.empty()) {
        WeexCore::WeexCoreManager::Instance()->getPlatformBridge()->platform_side()->ReportException(page_id.c_str(), nullptr, err.c_str());
    }
}

std::string VNodeRenderManager::CreatePageWithContent(const std::string &input, const std::string &page_id, const std::string &options, const std::string &init_data, std::function<void(const char *)> exec_js) {
    InitVM();
#ifdef DEBUG
    auto start = std::chrono::steady_clock::now();
#endif
    ExecState *exec_state = new ExecState(g_vm);
    exec_states_.insert({page_id, exec_state});
    VNodeExecEnv::ImportExecEnv(exec_state);
    std::string err;
    json11::Json json = json11::Json::parse(input, err);
    if (!err.empty() || json.is_null()) {
        exec_state->context()->raw_source() = input;
    }
    else {
        exec_state->context()->raw_json() = json;
        VNodeExecEnv::ParseData(exec_state);
        VNodeExecEnv::ParseStyle(exec_state);
        VNodeExecEnv::ParseScript(exec_state);
    }
    if (init_data.length() > 0) {
        VNodeExecEnv::ImportExecData(exec_state, init_data);
    }
    exec_state->context()->page_id(page_id);
    //auto compile_start = std::chrono::steady_clock::now();
    exec_state->Compile(err);
    if (!err.empty()) {
        LOGE("DATA_RENDER, compile err: %s",err.c_str());
        return err;
    }
#ifdef DEBUG
    auto compile_post = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    LOGD("[DATA_RENDER], Compile time:[%lld]\n", compile_post.count());
#endif
    //auto exec_start = std::chrono::steady_clock::now();
    exec_state->Execute(err);
    if (!err.empty()) {
        LOGE("DATA_RENDER, exec err: %s",err.c_str());
        return err;
    }
    if (exec_state->context()->root() == NULL) {
        return err;
    }

    CreatePageInternal(page_id, exec_state->context()->root());
#ifdef DEBUG
    auto duration_post = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    LOGD("DATA_RENDER, All time %lld\n", duration_post.count());
#endif
    DownloadAndExecScript(exec_state, page_id, exec_js);
    return err;
}

void VNodeRenderManager::DownloadAndExecScript(
    ExecState *exec_state, const std::string& page_id,
    std::function<void(const char*)> exec_js) {
  // If script exists in json, run script into js vm
  const json11::Json& script_array =
    exec_state->context()->script_json();
  if (script_array.is_array()) {
    for (auto it = script_array.array_items().begin();
         it != script_array.array_items().end(); it++) {
      const json11::Json& script_obj = *it;
      auto src = script_obj["src"];
      auto content = script_obj["content"];
      auto callback1 = weex::base::MakeCopyable(
          [page_id = page_id, exec_js = exec_js, exec_state = exec_state](const char* result) {
            exec_js(result);
            auto root =
                VNodeRenderManager::GetInstance()->GetRootVNode(page_id);
            if (root && root->IsVirtualComponent()) {
              static_cast<weex::core::data_render::VComponent*>(root)
                  ->DispatchCreated();

                //fire event
                exec_state->set_exec_js_finished(true);
                const std::vector<std::vector<std::string>>& event_queue = exec_state->event_queue();
                for (auto args : event_queue) {
                    VNodeRenderManager::GetInstance()->FireEvent(args[0], args[1], args[2], args[3], args[4]);
                }
                exec_state->ClearEventQueue();
            }
          });
      // callback2, a wrap for callback1, will be post to script thread to
      // execute callback1
      auto callback2 = weex::base::MakeCopyable([callback = callback1](
                                                    const std::string& result) {
#ifdef OS_ANDROID
        WeexCoreManager::Instance()->script_thread()->message_loop()->PostTask(
            weex::base::MakeCopyable([result = std::move(result), callback]() {
              callback(result.c_str());
            }));
#else
          callback(result.c_str());
#endif
      });
      // If script is a url, first download the script, else run script
      // directly.
      if (content.is_string() && !content.string_value().empty()) {
        callback1(const_cast<char*>(content.string_value().c_str()));
      } else if (src.is_string()) {
        network::HttpModule http_module;
        http_module.Send(page_id.c_str(), src.string_value().c_str(),
                         callback2);
      }
    }
  }
}

bool VNodeRenderManager::RequireModule(std::string &name, std::string &result)
{
    bool finished = false;
    do {
        if (!modules_.size()) {
            break;
        }
        for (auto iter = modules_.begin(); iter != modules_.end(); iter++) {
            std::string error;
            auto module_info = json11::Json::parse(*iter, error);
            if (module_info.is_object()) {
                if (module_info[name].is_array()) {
                    result = *iter;
                    finished = true;
                    break;
                }
            }
        }
        
    } while (0);
    
    return finished;
}
    
void VNodeRenderManager::DestroyExecutor() {
    do {
        qking_executor_t executor = qking_get_current_executor();
        if (!executor) {
            break;
        }
        std::string page_id;
        auto iter = executors_.begin();
        for (; iter != executors_.end(); iter++) {
            if (iter->second == executor) {
                page_id = iter->first;
                break;
            }
        }
        if (page_id.empty()) {
            break;
        }
        auto callback = callback_managers_.find(page_id);
        if (callback != callback_managers_.end()){
            callback_managers_.erase(callback);
        }
        VNodeRenderContext *context = (VNodeRenderContext *)qking_get_external_context_from_executor(executor);
        try {
            rax::RaxElementFactory::DestroyFactory(page_id);
        }
        catch (std::exception &e) {
            LOGE("[exception]:=>rax destroy factory err => %s", e.what());
        }
        try {
            qking_destroy_executor(executor);
        }
        catch (std::exception &e) {
            LOGE("[exception]:=>qking destroy executor err => %s", e.what());
        }
        if (context) {
            delete context;
        }
        executors_.erase(iter);
        
    } while (0);
}

std::string VNodeRenderManager::CreatePageWithContent(const uint8_t *contents, size_t length, const std::string &page_id, const std::string &options, const std::string &env, const std::string &init_data, std::function<void(const char *)> exec_js) {
#ifdef DEBUG
    auto start = std::chrono::steady_clock::now();
#endif
    VNodeRenderContext *context = new VNodeRenderContext();
    qking_executor_t executor = qking_create_executor(context);
    executors_.insert({page_id, executor});
    RAX_NS::RaxElementFactory::CreateFactory(page_id);
    callback_managers_.insert(std::make_pair(page_id, std::unique_ptr<CallbackManager>(new CallbackManager)));
    VNodeQkingEnv::Register();
    context->page_id(page_id);
    std::string err;
    if (!string_from_qking_set_assembly_code(executor, (uint8_t *)contents, length, err)) {
        DestroyExecutor();
        return err;
    }
    if (init_data.length() > 0) {
        VNodeQkingEnv::RegisterVariable(init_data);
    }
    VNodeQkingEnv::RegisterVariable("WXEnvironment", env);
    VNodeQkingEnv::RegisterVariable("__weex_options__", "{}");//todo
#ifdef DEBUG
    auto decoder_post = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    LOGD("[DATA_RENDER], Decoder time:[%lld]\n", decoder_post.count());
#endif
    if (!string_from_qking_execute_code(executor, err)) {
        DestroyExecutor();
        return err;
    }
//    if (context->root() == NULL) {
//        err = "Root vonde is null";
//        return err;
//    }
//    CreatePageInternal(page_id, context->root());
#ifdef DEBUG
    auto duration_post = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    LOGD("[DATA_RENDER], All time:[%lld]\n", duration_post.count());
#endif
    return err;
}

void VNodeRenderManager::CreatePage(const char *contents, size_t length, const std::string& page_id, const std::string& options, const std::string &env, const std::string& init_data,std::function<void(const char *)> exec_js) {
    string err = CreatePageWithContent((const uint8_t *)contents, length, page_id, options, env, init_data,exec_js);
    if (!err.empty()) {
        WeexCore::WeexCoreManager::Instance()->getPlatformBridge()->platform_side()->ReportException(page_id.c_str(), nullptr, err.c_str());
    }
}
    
bool VNodeRenderManager::RefreshPageWithExecutor(const std::string& page_id,
                                     const std::string& init_data) {
    do {
        auto iter = executors_.find(page_id);
        if (iter == executors_.end()) {
            break;
        }
        qking_executor_t executor = iter->second;
        qking_set_current_executor(executor);
        RAX_NS::RaxElementFactory *factory = nullptr;
        try {
          factory = RAX_NS::RaxElementFactory::GetFactory(page_id);
        } catch (...) {
          LOGE("[exception]:=>qking GetRaxElementFactory err, page_id %s",page_id.c_str());
        }
        if (!factory) {
          break;
        }
        //no needs to release;
        qking_value_t qking_handler = factory->get_docuemnt_event_handler();
        qking_value_t args[2] = {qking_create_string_c("refresh"), qking_create_string_c(init_data.c_str())};
        qking_value_t ret = qking_create_undefined();
        bool is_error = false;
        try {
            ret = qking_call_function(qking_handler, qking_create_undefined(), args, 2);
        }
        catch (std::exception &e) {
            LOGE("[exception]:=>qking refresh page error: %s", e.what());
            is_error = true;
        }
        qking_release_value(args[0]);
        qking_release_value(args[1]);
        if (qking_value_is_error(ret)) {
          LOGE("RefreshPage event handle err: %s", string_from_qking_error(ret).c_str());
        }
        qking_release_value(ret);
        if (is_error) {
            DestroyExecutor();
        }
        WeexCore::WeexCoreManager::Instance()
          ->getPlatformBridge()
          ->platform_side()
          ->RefreshFinish(page_id.c_str(), nullptr, "");
      return true;
        
    } while (0);

    return false;
}

bool VNodeRenderManager::RefreshPage(const std::string& page_id,
                                     const std::string& init_data) {
    do {
        auto iter = executors_.find(page_id);
        if (iter != executors_.end()) {
            return RefreshPageWithExecutor(page_id, init_data);
        }
        auto it = exec_states_.find(page_id);
        if (it == exec_states_.end()) {
            break;
        }
        ExecState *exec_state = it->second;
        // If component exsit, refresh by component
        auto it_vnode = vnode_trees_.find(page_id);
        if (it_vnode == vnode_trees_.end()) {
            return false;
        }
        if (it_vnode->second->IsVirtualComponent()) {
            auto component = static_cast<VComponent*>(it_vnode->second);
            Value data = StringToValue(exec_state, init_data);
            component->UpdateData(&data);
            return true;
        }
        // Otherwise re-execute
        VNodeExecEnv::ImportExecData(exec_state, init_data);
        std::string err;
        exec_state->context()->Reset();
        exec_state->Execute(err);  // refresh root
        if (!err.empty()) {
            break;
        }
        if (exec_state->context()->root() == NULL) {
            break;
        }
        RefreshPageInternal(page_id, exec_state->context()->root());
        WeexCore::WeexCoreManager::Instance()
            ->getPlatformBridge()
            ->platform_side()
            ->RefreshFinish(page_id.c_str(), nullptr, "");
        return true;
        
    } while (0);

    return false;
}

bool VNodeRenderManager::ClosePage(const std::string& page_id) {auto it = exec_states_.find(page_id);
    ClosePageInternal(page_id);
    if (it == exec_states_.end()) {
        return false;
    }
    ExecState *exec_state = it->second;
    delete exec_state;
    exec_states_.erase(it);
    return true;
}
    
static uint32_t StringToUint32(const std::string &s) {
    uint32_t val = 0;
    std::stringstream ss;
    ss << s;
    ss >> val;
    return val;
}
    
void VNodeRenderManager::InvokeCallback(const std::string& page_id,
                                        const std::string& callback_id,
                                        const std::string& data,
                                        bool keep_alive) {
    auto iter = executors_.find(page_id);
    if (iter == executors_.end()) {
        LOGE("[data-render] InvokeCallback: executor not found: %s",page_id.c_str());
        return;
    }
    qking_executor_t executor = iter->second;
    CallbackManager *callbackManager = GetCallbackManager(executor);
    if (!callbackManager){
        LOGE("[data-render] InvokeCallback: callbackManager not found: %s",page_id.c_str());
        return;
    }
    qking_set_current_executor(executor);
    
    uint32_t callback_var = StringToUint32(callback_id);
    if (callback_var == 0) {
        //convert to id err;
        LOGE("[data-render] InvokeCallback: callback_id err: %s",callback_id.c_str());
        return;
    }
    // if there is exception, must destroy executor
    if (!callbackManager->Call(callback_var, data, keep_alive)) {
        DestroyExecutor();
    }
}
    
void VNodeRenderManager::FireEventWithExecutor(const std::string &page_id, const std::string &ref, const std::string &event,const std::string &args,const std::string &dom_changes)
{
    do {
        auto iter = executors_.find(page_id);
        if (iter == executors_.end()) {
            break;
        }
        qking_executor_t executor = iter->second;
        qking_set_current_executor(executor);
        RAX_NS::RaxElementFactory *factory = nullptr;
        try{
          factory = RAX_NS::RaxElementFactory::GetFactory(page_id);
        } catch (...) {
          LOGE("GetRaxElementFactory err, page_id %s",page_id.c_str());
        }
        if (!factory) {
          break;
        }
        if (ref == "__root") {
          //todo support root event.
          LOGW("FireEvent on __root: skip, event: %s, args: %s",event.c_str(),args.c_str());
          break;
        }
        RAX_NS::RaxElement *pElement = factory->GetComponent(ref);
        if (!pElement) {
          LOGE("FireEvent err: node not found for ref: %s",ref.c_str());
          break;
        }
        if (pElement->type()!=rax::RaxElementType::kNative) {
          LOGE("FireEvent err: node not a native node");
          break;
        }

        RAX_NS::RaxNativeElement *native_ele = dynamic_cast<RAX_NS::RaxNativeElement*>(pElement);
        
        const auto& events = native_ele->get_events();
        const auto& it = events.find(event);
        if (it == events.end()) {
          LOGE("FireEvent err: node event registry not found for event: %s",event.c_str());
          break;
        }

        if (!qking_value_is_function(it->second->get())) {
          LOGE("FireEvent err: node event handler not function, event: %s",event.c_str());
          break;
        }
        qking_value_t func_args = qking_json_parse((const qking_char_t *)args.c_str(), (qking_size_t)args.length());
        qking_value_t completion_value = qking_create_undefined();
        bool is_error = false;
        try {
            completion_value = qking_call_function(it->second->get(), qking_create_undefined(), &func_args, 1);
        }
        catch (std::exception &e) {
            LOGE("[exception]:=>qking fire event error: %s", e.what());
            is_error = true;
        }
        if (qking_value_is_error(completion_value)) {
            LOGE("FireEvent error, %s",string_from_qking_error(completion_value).c_str());
        } else {
            LOGE("FireEvent succ: event: %s",event.c_str());
        }
        qking_release_value(func_args);
        qking_release_value(completion_value);
        if (is_error) {
            DestroyExecutor();
        }
        
    } while (0);
}
        
void VNodeRenderManager::FireEvent(const std::string &page_id, const std::string &ref, const std::string &event,const std::string &args,const std::string &dom_changes) {
    do {
        FireEventWithExecutor(page_id, ref, event, args, dom_changes);
        auto iter = exec_states_.find(page_id);
        if (iter == exec_states_.end()) {
            break;
        }
        if (!iter->second->exec_js_finished()) {
            std::vector<std::string> fire_event = {page_id, ref, event, args, dom_changes};
            iter->second->AddEvent(fire_event);
            break;
        }
        auto node = vnode_trees_.find(page_id);
        if (node == vnode_trees_.end()) {
            break;
        }
        // TODO merge two way to fire event
        {
            // First way to fire event from VNode::OnEvent
            auto vnode = iter->second->context()->GetVNode(ref);
            if (vnode && vnode->event_params_map()) {
                auto hit_test = vnode->event_params_map()->find(event);
                if (hit_test != vnode->event_params_map()->end()) {
                    // If vnode has eat event, return.
                    vnode->OnEvent(event, args, dom_changes);
                    return;
                }
            }
        }

        // Second way to fire event from call vm func
        auto vnode = node->second->FindNode(ref);
        if (!vnode) {
            break;
        }
        auto iter_event = vnode->events()->find(event);
        if (iter_event == vnode->events()->end()) {
            break;
        }
        if (!iter_event->second) {
            break;
        }
        FuncState *func_state = nullptr;
        FuncInstance *func_inst = nullptr;
        ExecState *exec_state = iter->second;
        bool finder = false;
        for (auto iter : exec_state->class_factory()->stores()) {
            if (iter.first == iter_event->second) {
                if (iter.second == Value::Type::FUNC_INST) {
                    func_inst = reinterpret_cast<FuncInstance *>(iter.first);
                }
                finder = true;
            }
        }
        if (!finder) {
            func_state = reinterpret_cast<FuncState *>(iter_event->second);
        }
        if (!func_state && !func_inst) {
            break;
        }
        std::vector<Value> caller_args;
        if (func_inst) {
            func_state = func_inst->func_;
        }
        if (func_state->is_class_func() && vnode->inst()) {
            Value inst;
            SetCIValue(&inst, reinterpret_cast<GCObject *>(vnode->inst()));
            caller_args.push_back(inst);
        }
        caller_args.push_back(StringToValue(exec_state, args));
        if (func_inst) {
            exec_state->Call(func_inst, caller_args);
        }
        else {
            exec_state->Call(func_state, caller_args);
        }
        
    } while (0);
}

void VNodeRenderManager::CallNativeModule(qking_executor_t executor,
                                          const std::string& module,
                                          const std::string& method,
                                          const std::string& args, int argc) {
    for (auto iter = executors_.begin(); iter != executors_.end(); iter++) {
        if (iter->second == executor) {
            WeexCoreManager::Instance()
            ->getPlatformBridge()
            ->platform_side()
            ->CallNativeModule(iter->first.c_str(), module.c_str(),
                               method.c_str(),
                               args.length() > 0 ? args.c_str() : nullptr,
                               static_cast<int>(args.length()), nullptr, 0);
            break;
        }
    }
}
    
void VNodeRenderManager::WXLogNative(ExecState *exec_state, const std::string &info) {
    for (auto iter = exec_states_.begin(); iter != exec_states_.end(); iter++) {
        if (iter->second == exec_state) {
            WeexCoreManager::Instance()
            ->getPlatformBridge()
            ->platform_side()
            ->NativeLog(info.c_str());
            break;
        }
    }
}

void VNodeRenderManager::UpdateComponentData(const std::string& page_id,
                                             const char* cid,
                                             const std::string& json_data) {
  ExecState* exec_state = GetExecState(page_id);
  if (!exec_state) return;
  VComponent* component = exec_state->context()->GetComponent(atoi(cid));
  if (component) {
    Value value(StringToValue(exec_state, json_data));
    component->UpdateData(&value);
  }
}

void VNodeRenderManager::PatchVNode(ExecState *exec_state, VNode *v_node, VNode *new_node) {
    for (auto iter = exec_states_.begin(); iter != exec_states_.end(); iter++) {
        if (iter->second == exec_state) {
            Patch(iter->first, v_node, new_node);
            break;
        }
    }
}

void VNodeRenderManager::PatchVNode(qking_executor_t executor, VNode *v_node, VNode *new_node) {
    for (auto iter = executors_.begin(); iter != executors_.end(); iter++) {
        if (iter->second == executor) {
            Patch(iter->first, v_node, new_node);
            break;
        }
    }
}

bool SameNode(VNode* a, VNode* b) {
  if (a->IsVirtualComponent() && b->IsVirtualComponent()) {
    return static_cast<VComponent*>(a)->IsSameNode(static_cast<VComponent*>(b));
  } else {
    // todo to be more accurate
    return a->tag_name() == b->tag_name() && a->ref() == b->ref() &&
           a->IsVirtualComponent() == b->IsVirtualComponent();
  }
}

inline VNode* GetOrNull(vector<VNode*>& vec, int index) {
  if (index < 0 || index >= vec.size()) {
    return nullptr;
  }
  return vec[index];
}

void RemoveNodes(const string& pageId, vector<VNode*>& vec,
                 vector<VNode*>& ref_list, unsigned int start,
                 unsigned int end);

void AddNodes(const string& pageId, vector<VNode*>& vec,
              vector<VNode*>& ref_list, unsigned int start, unsigned int end);

inline vector<VNode*>::iterator IndexOf(vector<VNode*>& vec,
                                        const VNode* value) {
  return std::find(vec.begin(), vec.end(), value);
}

int MoveToBackOfRef(vector<VNode*>& ref_list, const VNode* move_ref,
                    const VNode* anchor_ref);

int MoveToFrontOfRef(vector<VNode*>& ref_list, const VNode* move_ref,
                     const VNode* anchor_ref);

int MoveElmToFrontOfNode(const string& page_id, vector<VNode*>& ref_list,
                         VNode* move_node, VNode* anchor_node);

int MoveElmToBackOfNode(const string& page_id, vector<VNode*>& ref_list,
                        VNode* move_node, VNode* anchor_node);

void CreateAndInsertElm(const string& page_id, VNode* node,
                        vector<VNode*>& ref_list, const VNode* ref);

void UpdateChildren(const string& page_id, VNode* old_node, VNode* new_node) {
  vector<VNode*>& old_children = *old_node->child_list();
  vector<VNode*>& new_children = *new_node->child_list();
  vector<VNode*> ref_list;
  map<string, unsigned int>* ref_to_index = nullptr;

  // ref
  for (auto begin = old_children.begin(); begin < old_children.end(); begin++) {
    ref_list.push_back((*begin));
  }

  int old_start = 0;
  int old_end = static_cast<int>(old_children.size()) - 1;
  int new_start = 0;
  int new_end = static_cast<int>(new_children.size()) - 1;
  VNode* old_start_node = GetOrNull(old_children, old_start);
  VNode* old_end_node = GetOrNull(old_children, old_end);
  VNode* new_start_node = GetOrNull(new_children, new_start);
  VNode* new_end_node = GetOrNull(new_children, new_end);

  while (old_start <= old_end && new_start <= new_end) {
    if (old_start_node == nullptr) {
      //++oldStart might larger than children size;
      old_start_node = GetOrNull(old_children, ++old_start);

    } else if (old_end_node == nullptr) {
      old_end_node = GetOrNull(old_children, --old_end);

    } else if (new_start_node == nullptr) {
      new_start_node = GetOrNull(new_children, ++new_start);
    } else if (new_end_node == nullptr) {
      new_end_node = GetOrNull(new_children, --new_end);
    } else if (SameNode(old_start_node, new_start_node)) {
      PatchVNode(page_id, old_start_node, new_start_node);
      old_start_node = GetOrNull(old_children, ++old_start);
      new_start_node = GetOrNull(new_children, ++new_start);

    } else if (SameNode(old_end_node, new_end_node)) {
      PatchVNode(page_id, old_end_node, new_end_node);
      old_end_node = GetOrNull(old_children, --old_end);
      new_end_node = GetOrNull(new_children, --new_end);

    } else if (SameNode(old_start_node, new_end_node)) {
      PatchVNode(page_id, old_start_node, new_end_node);
      MoveElmToBackOfNode(page_id, ref_list, old_start_node, old_end_node);
      old_start_node = GetOrNull(old_children, ++old_start);
      new_end_node = GetOrNull(new_children, --new_end);

    } else if (SameNode(old_end_node, new_start_node)) {
      PatchVNode(page_id, old_end_node, new_start_node);
      MoveElmToFrontOfNode(page_id, ref_list, old_end_node, old_start_node);
      old_end_node = GetOrNull(old_children, --old_end);
      new_start_node = GetOrNull(new_children, ++new_start);
    } else {
      // create on first time
      if (ref_to_index == nullptr) {
        ref_to_index = new map<string, unsigned int>();
        for (unsigned int i = old_start; i <= old_end; ++i) {
          auto vnode = GetOrNull(old_children, i);
          if (vnode == nullptr || vnode->ref().empty()) {
            continue;
          }
          ref_to_index->insert({vnode->ref(), i});
        }
      }

      auto pos = ref_to_index->find(new_start_node->ref());
      if (pos == ref_to_index->end()) {
        // no node found, create new
        CreateAndInsertElm(page_id, new_start_node, ref_list, old_start_node);
        new_start_node = GetOrNull(new_children, ++new_start);
      } else {
        auto node_to_move = GetOrNull(old_children, pos->second);
        if (node_to_move == nullptr) {
          // wtf!
#if VRENDER_LOG
          LOGE("[VRenderManager] already moved, has duplicated ref: %s",
               new_start_node->ref().c_str());
#endif
          continue;
        }
        if (SameNode(node_to_move, new_start_node)) {
          PatchVNode(page_id, node_to_move, new_start_node);

          // delete old node.
          *IndexOf(ref_list, node_to_move) = new_start_node;
          delete old_children[pos->second];
          old_children[pos->second] = nullptr;

          MoveElmToFrontOfNode(page_id, ref_list, new_start_node,
                               old_start_node);
          new_start_node = GetOrNull(new_children, ++new_start);
        } else {
          // same ref exist, after insert there will be duplicated ref.
          // so must remove it in real dom first.
          RenderManager::GetInstance()->RemoveRenderObject(
              page_id, node_to_move->render_object_ref());
          ref_list.erase(IndexOf(ref_list, node_to_move));
          delete old_children[pos->second];
          old_children[pos->second] = nullptr;
          // no need to remove from keyToIndex, will go into wtf above;

          CreateAndInsertElm(page_id, new_start_node, ref_list, old_start_node);
          new_start_node = GetOrNull(new_children, ++new_start);
        }
      }
    }
  }

  if (old_start > old_end) {
    AddNodes(page_id, new_children, ref_list, new_start,
             new_end);  // end is include
  } else if (new_start > new_end) {
    RemoveNodes(page_id, old_children, ref_list, old_start, old_end);
  }

  if (ref_to_index != nullptr) {
    delete ref_to_index;
    ref_to_index = nullptr;
  }
}

void CreateAndInsertElm(const string& page_id, VNode* node,
                        vector<VNode*>& ref_list, const VNode* ref) {
  auto insert_pos = IndexOf(ref_list, ref);
  int index = static_cast<int>(std::distance(ref_list.begin(), insert_pos));
  ref_list.insert(insert_pos, node);

  WeexCore::RenderObject* root = ParseVNode2RenderObject(node, nullptr, false, 0, page_id);
  RenderManager::GetInstance()->AddRenderObject(
      page_id, node->parent()->render_object_ref(), index, root);
}

int MoveToBackOfRef(vector<VNode*>& ref_list, const VNode* move_ref,
                    const VNode* anchor_ref) {
  auto move_pos = IndexOf(ref_list, move_ref);
  auto anchor_pos = IndexOf(ref_list, anchor_ref);
  if (move_pos == ref_list.end()) {
#if VRENDER_LOG
    LOGE("[VRenderManager] moveToBackOfRef movePos == refList.end() ref: %s",
         move_ref->ref().c_str());
#endif
    return -1;  // wtf!
  }
  if (anchor_pos == ref_list.end()) {
#if VRENDER_LOG
    LOGE("[VRenderManager] moveToBackOfRef anchorPos == refList.end() ref: %s",
         anchor_ref->ref().c_str());
#endif
    return -1;  // wtf
  }
  int index = static_cast<int>(std::distance(ref_list.begin(), anchor_pos));
  VNode* value = *move_pos;
  ref_list.erase(move_pos);
  ref_list.insert(anchor_pos, value);
  return index;
}

int MoveToFrontOfRef(vector<VNode*>& ref_list, const VNode* move_ref,
                     const VNode* anchor_ref) {
  auto move_pos = IndexOf(ref_list, move_ref);
  auto anchor_pos = IndexOf(ref_list, anchor_ref);
  if (move_pos == ref_list.end()) {
#if VRENDER_LOG
    LOGE("[VRenderManager] moveToFrontOfRef movePos == refList.end() ref: %s",
         move_ref->ref().c_str());
#endif
    return -1;  // wtf!
  }
  if (anchor_pos == ref_list.end()) {
#if VRENDER_LOG
    LOGE("[VRenderManager] moveToFrontOfRef anchorPos == refList.end() ref: %s",
         anchor_ref->ref().c_str());
#endif
    return -1;  // wtf
  }
  int index = static_cast<int>(std::distance(ref_list.begin(), anchor_pos));
  VNode* value = *move_pos;
  ref_list.erase(move_pos);
  ref_list.insert(anchor_pos, value);
  return index;
}

int MoveElmToFrontOfNode(const string& page_id, vector<VNode*>& ref_list,
                         VNode* move_node, VNode* anchor_node) {
  int move_to_index = MoveToFrontOfRef(ref_list, move_node, anchor_node);
  RenderManager::GetInstance()->MoveRenderObject(
      page_id, move_node->render_object_ref(),
      move_node->parent()->render_object_ref(), move_to_index);
  return move_to_index;
}

int MoveElmToBackOfNode(const string& page_id, vector<VNode*>& ref_list,
                        VNode* move_node, VNode* anchor_node) {
  int move_to_index = MoveToBackOfRef(ref_list, move_node, anchor_node);
  RenderManager::GetInstance()->MoveRenderObject(
      page_id, move_node->render_object_ref(),
      move_node->parent()->render_object_ref(), move_to_index);
  return move_to_index;
}

void AddNodes(const string& pageId, vector<VNode*>& vec,
              vector<VNode*>& ref_list, unsigned int start, unsigned int end) {
  for (int i = start; i <= end; ++i) {
    auto p_node = vec[i];
    ref_list.insert(ref_list.begin() + i, p_node);
    WeexCore::RenderObject *node = ParseVNode2RenderObject(p_node, nullptr, false, 0, pageId);
    RenderManager::GetInstance()->AddRenderObject(
        pageId, p_node->parent()->render_object_ref(), i, node);
  }
}

void RemoveNodes(const string& pageId, vector<VNode*>& vec,
                 vector<VNode*>& ref_list, unsigned int start,
                 unsigned int end) {
  for (int i = start; i <= end; ++i) {
    auto p_node = vec[i];
    // some might already been used for patch, which is null.
    if (p_node == nullptr) {
      continue;
    }
    auto pos = IndexOf(ref_list, p_node);
    if (pos == ref_list.end()) {
      // wtf???
#if VRENDER_LOG
      LOGE("[VRenderManager] removeNodes pos == refList.end() ref: %s",
           p_node->ref().c_str());
#endif
      continue;
    }

    RenderManager::GetInstance()->RemoveRenderObject(
        pageId, (*pos)->render_object_ref());
  }
}

vector<pair<string, string>>* CompareMap(const map<string, string>& oldMap,
                                         const map<string, string>& newMap) {
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

void CompareAndApplyEvents1(const std::string& page_id, VNode* old_node,
                            VNode* new_node) {
  std::map<std::string, void*> old_events = *old_node->events();
  std::map<std::string, void*> new_events = *new_node->events();
  std::map<std::string, void*> remove_events;
  std::map<std::string, void*> add_events;

  for (auto it = old_events.cbegin(); it != old_events.cend(); it++) {
    auto pos = new_events.find(it->first);
    if (pos == new_events.end()) {
      remove_events.insert(*it);
    }
  }
  for (auto it = new_events.cbegin(); it != new_events.cend(); it++) {
    auto pos = old_events.find(it->first);
    if (pos == old_events.end()) {
      add_events.insert(*it);
    }
  }
  for (auto it = remove_events.cbegin(); it != remove_events.cend(); it++) {
    RenderManager::GetInstance()->RemoveEvent(
        page_id, new_node->render_object_ref(), it->first);
  }
  for (auto it = add_events.cbegin(); it != add_events.cend(); it++) {
    RenderManager::GetInstance()->AddEvent(
        page_id, new_node->render_object_ref(), it->first);
  }
}

void CompareAndApplyEvents2(const std::string& page_id, VNode* old_node,
                            VNode* new_node) {
  VNode::EventParamsMap old_events = *old_node->event_params_map();
  VNode::EventParamsMap new_events = *new_node->event_params_map();

  VNode::EventParamsMap remove_events;
  VNode::EventParamsMap add_events;
  for (auto it = old_events.cbegin(); it != old_events.cend(); it++) {
    auto pos = new_events.find(it->first);

    if (pos == new_events.end()) {
        remove_events.insert(*it);
    }
  }
  for (auto it = new_events.cbegin(); it != new_events.cend(); it++) {
    auto pos = old_events.find(it->first);

    if (pos == old_events.end()) {
        add_events.insert(*it);
    }
  }
  for (auto it = remove_events.cbegin(); it != remove_events.cend(); it++) {
    RenderManager::GetInstance()->RemoveEvent(
        page_id, new_node->render_object_ref(), it->first);
  }
  for (auto it = add_events.cbegin(); it != add_events.cend(); it++) {
    RenderManager::GetInstance()->AddEvent(
        page_id, new_node->render_object_ref(), it->first);
  }
}

void PatchVNode(const string& page_id, VNode* old_node, VNode* new_node) {
  if (old_node->IsVirtualComponent()) {
    static_cast<VComponent*>(old_node)
        ->MoveTo(static_cast<VComponent*>(new_node));
    return;
  }

  // patch render object link
  new_node->set_render_object_ref(old_node->render_object_ref());

  // compare attr, ptr will be delete by RenderPage
  auto p_vec = CompareMap(*(old_node->attributes()), *(new_node->attributes()));
  if (p_vec->size() > 0) {
      RenderManager::GetInstance()->UpdateAttr(page_id, new_node->render_object_ref(), p_vec);
  } else {
      delete p_vec;
      p_vec = nullptr;
  }
  // compare style, ptr will be delete by RenderPage
  p_vec = CompareMap(*(old_node->styles()), *(new_node->styles()));
  if (p_vec->size()) {
      RenderManager::GetInstance()->UpdateStyle(page_id, new_node->render_object_ref(), p_vec);
  } else {
      delete p_vec;
      p_vec = nullptr;
  }

  // compare and apply event
  CompareAndApplyEvents1(page_id, old_node, new_node);
  CompareAndApplyEvents2(page_id, old_node, new_node);

  // compare children
  if (old_node->HasChildren() && new_node->HasChildren()) {
    UpdateChildren(page_id, old_node, new_node);
  } else if (old_node->HasChildren() && !new_node->HasChildren()) {
    for (auto i = old_node->child_list()->cbegin();
         i != old_node->child_list()->cend(); i++) {
      RenderManager::GetInstance()->RemoveRenderObject(
          page_id, (*i)->render_object_ref());
    }
  } else if (!old_node->HasChildren() && new_node->HasChildren()) {
    int index = 0;
    for (auto it = new_node->child_list()->cbegin();
         it != new_node->child_list()->cend(); it++) {
      WeexCore::RenderObject *child_node = ParseVNode2RenderObject(*it, nullptr, false, 0, page_id);
      RenderManager::GetInstance()->AddRenderObject(
                                                      page_id, (*it)->parent()->render_object_ref(), index, child_node);
      ++index;
    }
  }
}

void Patch(const string& page_id, VNode *old_node, VNode *new_node) {
    if (!old_node) {
        ParseVNode2RenderObject(new_node, nullptr, false, 0, page_id);
    } else if (old_node->parent() == NULL || SameNode(old_node, new_node)) {
        // root must be the same;
        PatchVNode(page_id, old_node, new_node);
    } else {
        WeexCore::RenderObject *new_render_object = ParseVNode2RenderObject(new_node, nullptr, false, 0, page_id);
        VNode *parent = (VNode *)old_node->parent();
        if (!parent && old_node->component()) {
            parent = const_cast<VNode*>(old_node->component()->parent());
            old_node = old_node->component();
        }
        if (parent) {
            vector<VNode *> &old_children = *parent->child_list();
            auto pos = std::find(old_children.begin(), old_children.end(), old_node);
            int index = static_cast<int>(std::distance(old_children.begin(), pos));
            parent->InsertChild(new_node, index);
            RenderManager::GetInstance()->AddRenderObject(page_id, parent->render_object_ref(), index, new_render_object);
            parent->RemoveChild(old_node);
            RenderManager::GetInstance()->RemoveRenderObject(page_id, old_node->render_object_ref());
        }
    }
}

}  // namespace data_render
}  // namespace core
}  // namespace weex
