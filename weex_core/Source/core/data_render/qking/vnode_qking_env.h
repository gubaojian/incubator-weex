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

#ifndef CORE_DATA_RENDER_VNODE_VNODE_QKING_EXEC_ENV_H
#define CORE_DATA_RENDER_VNODE_VNODE_QKING_EXEC_ENV_H

#include "include/qking.h"

namespace weex {
namespace core {
namespace data_render {
    
class CallbackManager {
public:
    uint32_t AddCallback(qking_value_t callback);
    void Call(uint32_t id, const std::string &data,bool keep_alive);
private:
    uint32_t callback_id_ = 0;
    std::map<uint32_t, qking_value_t> callback_map_;
};
    
class VNodeQkingEnv {
 public:
  static void Register();
  static void RegisterVariable(const std::string &init_data_str);
  static void RegisterVariable(const char *name_p, const std::string &jsonstr);
};
    
//Value StringToValue(ExecState *exec_state, const std::string &str);

}  // namespace data_render
}  // namespace core
}  // namespace weex

#endif  // CORE_DATA_RENDER_VNODE_VNODE_QKING_EXEC_ENV_H
