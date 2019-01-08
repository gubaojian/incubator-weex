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
// Created by Xu Jiacheng on 2018/12/25.
//

#ifndef QKING_ROOT_RAX_COMMON_H
#define QKING_ROOT_RAX_COMMON_H

#include <exception>
#include <cstdlib>
#include <string>
#include "qking/core/jrt/jrt.h"
#include "third_party/json11/json11.hpp"
#include "base/common.h"
#include "base/log_defines.h"
#include "qking/include/qking.h"
#include "core/render/node/render_object.h"

#define RAX_NAME_SPACE_BEGIN \
  namespace weex {           \
  namespace core {           \
  namespace data_render {    \
  namespace rax {

#define RAX_NAME_SPACE_END \
  }                        \
  }                        \
  }                        \
  }

#define RAX_NS ::weex::core::data_render::rax

/*
 * Make sure unused parameters, variables, or expressions trigger no compiler
 * warning.
 */
#define RAX_UNUSED(x) ((void)(x))

#define RAX_MAKESURE(expr)                                                 \
  do {                                                                     \
    if (!(expr)) {                                                         \
      LOGE("RAX: Exception '%s' failed at %s(%s):%lu.\n", #expr, __FILE__, \
           __func__, (unsigned long)__LINE__);                             \
      QKING_ASSERT(0);                                                 \
    }                                                                      \
  } while (0)

#define RAX_ASSERT(expr)  QKING_ASSERT(expr)

using native_node_ptr = ::WeexCore::RenderObject *;

RAX_NAME_SPACE_BEGIN
std::string
rax_get_current_page_name();  // should use extern context to get name
RAX_NAME_SPACE_END

using namespace json11;

#endif  // QKING_ROOT_RAX_COMMON_H
