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

#include "core/jrt/jrt.h"
#include "vm_op_code.h"
#include "base/common_logger.h"

#define T(name, string, ops) ops,
static const int8_t s_ops_[NUM_OPCODES] = {OP_CODE_LIST(T)};
#undef T

int qking_oputil_get_ops(OPCode code){
  QKING_ASSERT(code < NUM_OPCODES);  // token is unsigned.
  return s_ops_[code];
}

bool qking_opuitl_is_jump_code(OPCode code) {
    return code == OP_RETURN0 || code == OP_RETURN1 || code == OP_GOTO || code == OP_JMP_PC || code == OP_N_JMP;
}

#ifndef QKING_NDEBUG

#define T(name, string, ops) #name,
static const char *const s_name_[NUM_OPCODES] = {OP_CODE_LIST(T)};
#undef T

const char *qking_oputil_get_name(OPCode code){
  QKING_ASSERT(code < NUM_OPCODES);
  return s_name_[code];
}

void qking_oputil_print_code(Instruction instruction, size_t pc) {
  OPCode code = GET_OP_CODE(instruction);
  const char *op_code = qking_oputil_get_name(code);
  int8_t ops = s_ops_[code];
  if (ops == 1) {
    LOGD("PC:%3u %-20s A:%3ld", pc, op_code, GET_ARG_Ax(instruction));
  } else if (ops == 2) {
    LOGD("PC:%3u %-20s A:%3ld B:%3ld", pc, op_code, GET_ARG_A(instruction),
         GET_ARG_Bx(instruction));
  } else if (ops == 3) {
    LOGD("PC:%3u %-20s A:%3ld B:%3ld C:%3ld", pc, op_code,
         GET_ARG_A(instruction), GET_ARG_B(instruction),
         GET_ARG_C(instruction));
  } else if (ops == 0) {
    LOGD("PC:%3u %-20s", pc, op_code);
  } else {
    assert(false);
  }
}

#endif
