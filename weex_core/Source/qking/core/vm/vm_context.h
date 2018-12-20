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
#ifndef VM_CONTEXT_H
#define VM_CONTEXT_H

#ifndef CONFIG_DISABLE_COMPILER_BUILTIN

#include <memory>
#include <string>
#include "third_party/json11/json11.hpp"

namespace quick {
namespace king {

class VMContext {
 public:
  VMContext() : raw_json_(){};
  virtual ~VMContext(){};
  inline json12::Json& raw_json() { return raw_json_; }
  inline std::string& raw_source() { return raw_source_; }

 private:
  json12::Json raw_json_;
  std::string raw_source_;
};

}  // namespace king
}  // namespace quick

#endif

#endif  // VM_CONTEXT_H
