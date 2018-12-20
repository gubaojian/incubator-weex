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
// Created by pentao.pt on 2018/7/25.
//

#ifndef QKING_COMMON_ERROR_H
#define QKING_COMMON_ERROR_H

#include <stdexcept>
#include "base/string_utils.h"
#ifndef CONFIG_DISABLE_COMPILER_BUILTIN
#include "core/parser/rax/token.h"
#endif

namespace quick {
namespace king {

class Error : public std::runtime_error {
 public:
  Error(std::string prefix, std::string msg)
      : std::runtime_error(std::move(msg)), prefix_{std::move(prefix)} {}

  const char *what() const noexcept override {
    static std::string result;
    result = (prefix_ + ": " + std::runtime_error::what());
    return result.c_str();
  }

 private:
  std::string prefix_;
};

class GeneratorError : public Error {
 public:
  GeneratorError(std::string str) : Error("[GeneratorError]", std::move(str)) {}
};

class JSError : public Error {
 public:
  JSError(std::string prefix, std::string str)
      : Error("[JSError]=>" + prefix, std::move(str)) {}
};

class VMExecError : public Error {
 public:
  VMExecError(std::string str) : Error("[VMExecError]=>", std::move(str)) {}
};

class EncoderError : public Error {
 public:
  EncoderError(std::string str) : Error("[EncoderError]=>", std::move(str)) {}
};

class OpcodeDecodeError : public Error {
 public:
  OpcodeDecodeError(std::string str)
      : Error("[OpcodeDecodeError]", std::move(str)) {}
};

class DecoderError : public Error {
 public:
  DecoderError(std::string str) : Error("[DecoderError]=>", std::move(str)) {}
};

#ifndef CONFIG_DISABLE_COMPILER_BUILTIN

class SyntaxError : public JSError {
 public:
  SyntaxError(Token token, std::string str = "")
      : JSError("SyntaxError(" + to_string(token.position().row() + 1) + ":" +
                    to_string(token.position().col()) + ") '" + token.view() +
                    "'",
                std::move(str)),
        token_(token) {}

  const char *what() const noexcept override { return JSError::what(); }

  Token &token() { return token_; }

 private:
  Token token_;
};

#endif

class TypeError : public JSError {
 public:
  TypeError(std::string msg = "") : JSError("TypeError", std::move(msg)) {}

  const char *what() const noexcept override { return JSError::what(); }
};

class ReferenceError : public JSError {
 public:
  ReferenceError(std::string msg = "")
      : JSError("ReferenceError", std::move(msg)) {}

  const char *what() const noexcept override { return JSError::what(); }
};

class RangeError : public JSError {
 public:
  RangeError(std::string msg = "") : JSError("RangeError", std::move(msg)) {}
};

}  // namespace king
}  // namespace quick

#endif  // QKING_COMMON_ERROR_H
