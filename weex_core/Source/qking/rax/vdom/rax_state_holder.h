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
//
// Created by Xu Jiacheng on 2018-12-29.
//

#ifndef QKING_ROOT_RAX_STATE_HOLDER_H
#define QKING_ROOT_RAX_STATE_HOLDER_H
#include <queue>
#include "rax_core_util.h"
RAX_NAME_SPACE_BEGIN

class RaxStateHolder {
 public:
  inline bool state_queue_empty() { return state_queue_.empty(); }

  inline void state_queue_push(qking_value_t value) {
    state_queue_.emplace(new QKValueRef(value));
  };

  qking_value_t state_queue_pop();

 private:
  std::queue<std::unique_ptr<QKValueRef>> state_queue_;
};

RAX_NAME_SPACE_END
#endif  // QKING_ROOT_RAX_STATE_HOLDER_H
