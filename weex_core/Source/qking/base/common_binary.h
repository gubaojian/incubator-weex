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
// Created by Xu Jiacheng on 2018/12/3.
//

#ifndef QKING_COMPILER_BINARY_COMMON_H
#define QKING_COMPILER_BINARY_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SECTION_VALUE_MASK 0x03
#define SECTION_EXT_MASK 0x80
#define SECTION_TAG_MASK 0x10
#define SECTION_LEN_MASK 0x0c
#define SECTION_LEN_OFF 0x2
#define SECTION_TEST_FLAG(flag, mask) (flag & mask)
#define SECTION_GET_LENGTH_LEN(flag, len)                \
  len = ((flags & SECTION_LEN_MASK) >> SECTION_LEN_OFF); \
  if (len == 3) len = 4;

#define SECTION_FUNCTION_MAXARGS 256
#define SECTION_VALUE_FINISHED 0xff
#define SECTION_SET_LENGTH_LEN_FLAG(flag, LenLen) \
  if (LenLen == 4) LenLen = 3;                    \
  flag |= (LenLen << SECTION_LEN_OFF);
#define SECTION_REGISTER_MAX 255

#define EXEC_BINARY_MAGIC_NUMBER 0x6d736100
#define EXEC_BINARY_CURRENT_VERSION 8
#define EXEC_BINARY_COMPATIBLE_VERSION 8
#define EXEC_BINARY_ENCRYPT 0

typedef enum {
  EXEC_SECTION_NONE = 0,
  EXEC_SECTION_HEADER,
  EXEC_SECTION_STRING,
  EXEC_SECTION_FUNCTION,
  EXEC_SECTION_VALUEREF,
} ExecSection;

typedef enum {
  ENCODE_VALUE_TYPE_INTEGER = 0,
  ENCODE_VALUE_TYPE_NUMBER = 1,
  ENCODE_VALUE_TYPE_STRING = 2,
  ENCODE_VALUE_TYPE_STRING_DIRECT = 3,
  ENCODE_VALUE_TYPE_REGEX = 4,
  ENCODE_VALUE_TYPE_REGEX_DIRECT = 5,
  ENCODE_VALUE_TYPE_BOOLEAN = 6,  // make it op_code
} EncodeValueType;

typedef enum {
  kHeaderMagicCode,
  kHeaderVersion,
  kHeaderCompartibleVersion,
  kHeaderEncrypt,
} EncodeHeaderType;

typedef enum {
  kValueStringSize,
  kValueStringPayload,
  kValueStringFlagSize,
  kValueStringFlagPayload,
} SectionStringKey;

typedef enum {
  kValueFunctionSize,
  kValueFunctionSuper,
  kValueFunctionArgc,
  kValueFunctionStackSize,
  kValueFunctionContextDepth,
  kValueFunctionStatusFlag,
  kValueFunctionInClosure,
  kValueFunctionOutClosure,
  kValueFunctionInstructions,
  kValueFunctionConstantCount,
  kValueFunctionConstantPayload,
  kValueFunctionFinished = 255,
} SectionFunctionKey;

typedef enum {
  kValueOutRefSize,
  kValueOutRefFuncStateIndex,
  kValueOutRefIdx,
  kValueOutRefFinished,
} SectionValueOutRefKey;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // QKING_COMPILER_BINARY_COMMON_H
