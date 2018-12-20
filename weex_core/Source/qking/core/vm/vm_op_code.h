/*
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

#ifndef VM_OP_CODE_H
#define VM_OP_CODE_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define OP_CODE_LIST(T)                                      \
  T(OP_MOVE, "OP_MOVE", 2)                                   \
  T(OP_LOADK, "OP_LOADK", 2)                                 \
  T(OP_CALL, "OP_CALL", 3)                                   \
  T(OP_NEW, "OP_NEW", 3)                                     \
  T(OP_GET_PROPERTY, "OP_GET_PROPERTY", 3)                   \
  T(OP_SET_PROPERTY, "OP_SET_PROPERTY", 3)                   \
  T(OP_LOAD_UNDEFINED, "OP_LOAD_UNDEFINED", 1)               \
  T(OP_LOAD_NULL, "OP_LOAD_NULL", 1)                         \
  T(OP_GET_GLOBAL, "OP_GET_GLOBAL", 2)                       \
  T(OP_GET_BY_THIS_OR_GLOBAL, "OP_GET_BY_THIS_OR_GLOBAL", 3) \
  T(OP_GET_LOCAL, "OP_GET_LOCAL", 2)                         \
  T(OP_INHERITANCE, "OP_INHERITANCE", 2)                     \
  T(OP_CONSTRUCTOR, "OP_CONSTRUCTOR", 3)                     \
  T(OP_GET_SUPER, "OP_GET_SUPER", 2)                         \
  T(OP_OUT_CLOSURE, "OP_OUT_CLOSURE", 1)                     \
  T(OP_TYPEOF, "OP_TYPEOF", 2)                               \
  T(OP_ADD, "OP_ADD", 3)                                     \
  T(OP_SUB, "OP_SUB", 3)                                     \
  T(OP_MUL, "OP_MUL", 3)                                     \
  T(OP_MOD, "OP_MOD", 3)                                     \
  T(OP_DIV, "OP_DIV", 3)                                     \
  T(OP_N_JMP, "OP_N_JMP", 2)                                 \
  T(OP_JMP_PC, "OP_JMP_PC", 2)                               \
  T(OP_GOTO, "OP_GOTO", 1)                                   \
  T(OP_EQUAL, "OP_EQUAL", 3)                                 \
  T(OP_STRICT_EQUAL, "OP_STRICT_EQUAL", 3)                   \
  T(OP_LESS, "OP_LESS", 3)                                   \
  T(OP_LESS_EQUAL, "OP_LESS_EQUAL", 3)                       \
  T(OP_GREATER, "OP_GREATER", 3)                             \
  T(OP_GREATER_EQUAL, "OP_GREATER_EQUAL", 3)                 \
  T(OP_FOR_IN, "OP_FOR_IN", 3)                               \
  T(OP_IN, "OP_IN", 3)                                       \
  T(OP_NOT, "OP_NOT", 2)                                     \
  T(OP_INSTANCEOF, "OP_INSTANCEOF", 3)                       \
  T(OP_PRE_INCR, "OP_PRE_INCR", 2)                           \
  T(OP_PRE_DECR, "OP_PRE_DECR", 2)                           \
  T(OP_POST_INCR, "OP_POST_INCR", 2)                         \
  T(OP_POST_DECR, "OP_POST_DECR", 2)                         \
  T(OP_BIT_AND, "OP_BIT_AND", 3)                             \
  T(OP_BIT_OR, "OP_BIT_OR", 3)                               \
  T(OP_BIT_XOR, "OP_BIT_XOR", 3)                             \
  T(OP_BIT_NOT, "OP_BIT_NOT", 2)                             \
  T(OP_LEFT_SHIFT, "OP_LEFT_SHIFT", 3)                       \
  T(OP_RIGHT_SHIFT, "OP_RIGHT_SHIFT", 3)                     \
  T(OP_ZERO_RIGHT_SHIFT, "OP_ZERO_RIGHT_SHIFT", 3)           \
  T(OP_UNS, "OP_UNS", 3)                                     \
  T(OP_RETURN0, "OP_RETURN0", 0)                             \
  T(OP_RETURN1, "OP_RETURN1", 1)                             \
  T(OP_THROW, "OP_THROW", 1)                                 \
  T(OP_TRY, "OP_TRY", 1)                                     \
  T(OP_CATCH, "OP_CATCH", 2)                                 \
  T(OP_FINALLY, "OP_FINALLY", 1)                             \
  T(OP_POP_CONTEXT, "OP_POP_CONTEXT", 2)                     \
  T(OP_DELETE, "OP_DELETE", 2)                               \
  T(OP_DELETE_PROPERTY, "OP_DELETE_PROPERTY", 3)             \
  T(OP_SET_GETTER, "OP_SET_GETTER", 3)                       \
  T(OP_SET_SETTER, "OP_SET_SETTER", 3)                       \
  T(OP_INVALID, "OP_INVALID", 0)

#define T(name, string, ops) name,
typedef enum { OP_CODE_LIST(T) NUM_OPCODES } OPCode;
#undef T

typedef unsigned long Instruction;

int qking_oputil_get_ops(OPCode code);
bool qking_opuitl_is_jump_code(OPCode code);

#ifndef QKING_NDEBUG

const char *qking_oputil_get_name(OPCode code);

void qking_oputil_print_code(Instruction instruction, size_t pc);

#endif

typedef enum {
  VM_OP_NEW_TYPE_MAP = 0,
  VM_OP_NEW_TYPE_ARRAY = 1,
  VM_OP_NEW_TYPE_FUNCTION = 2,
  VM_OP_NEW_TYPE_OBJECT = 3,
  VM_OP_NEW_TYPE_CONSTRUCTOR = 4,
  VM_OP_NEW_TYPE_IMPL_CONSTRUCTOR = 5,
  VM_OP_NEW_TYPE_IMPL_SUPER_CONSTRUCTOR = 6,
  VM_OP_NEW_TYPE_MAX /**< maximum value */

} vm_op_new_type_t;

typedef enum {
  VM_OP_GET_SUPER_TYPE_CONSTRUCTOR = 0,
  VM_OP_GET_SUPER_TYPE_PROTOTYPE = 1,
  VM_OP_GET_SUPER_TYPE_MAX /**< maximum value */

} vm_op_get_super_type_t;

typedef enum {
  VM_OP_PUSH_CONTEXT_TYPE_TRY = 0,
  VM_OP_PUSH_CONTEXT_TYPE_CATCH = 1,
  VM_OP_PUSH_CONTEXT_TYPE_FINALLY = 2,
  VM_OP_PUSH_CONTEXT_TYPE_MAX /**< maximum value */

} vm_op_push_context_type_t;

typedef enum {
  VM_OP_POP_CONTEXT_TYPE_END = 0,
  VM_OP_POP_CONTEXT_TYPE_JUMP = 1,
  VM_OP_POP_CONTEXT_TYPE_MAX /**< maximum value */
} vm_op_pop_context_type_t;

#define SIZE_OP 8
#define SIZE_C 8
#define SIZE_B 8
#define SIZE_A 8
#define SIZE_Ax (SIZE_A + SIZE_B + SIZE_C)
#define SIZE_Bx (SIZE_B + SIZE_C)

#define POS_OP 0
#define POS_A (POS_OP + SIZE_OP)
#define POS_Ax POS_A
#define POS_B (POS_A + SIZE_A)
#define POS_Bx (POS_B)
#define POS_C (POS_B + SIZE_B)

#define CREATE_ABC(op_code, a, b, c)                                     \
  a < 0 || b < 0 || c < 0                                                \
      ? ((Instruction)OP_INVALID << POS_OP)                              \
      : ((Instruction)op_code << POS_OP) | ((Instruction)(a) << POS_A) | \
            ((Instruction)(b) << POS_B) | ((Instruction)(c) << POS_C)

#define CREATE_Ax(op_code, ax)                 \
  ax < 0 ? ((Instruction)OP_INVALID << POS_OP) \
         : ((Instruction)op_code << POS_OP) | ((Instruction)(ax) << POS_Ax)

#define CREATE_ABx(op_code, a, bx)                                       \
  a < 0 || bx < 0                                                        \
      ? ((Instruction)OP_INVALID << POS_OP)                              \
      : ((Instruction)op_code << POS_OP) | ((Instruction)(a) << POS_A) | \
            ((Instruction)(bx) << POS_Bx)

#define CREATE_SLOT \
  (((Instruction)OP_INVALID << POS_OP) | ((Instruction)(1) << POS_Ax))

#define GET_OP_CODE(i) (OPCode)(((i) >> POS_OP) & 0xFF)
#define GET_ARG_A(i) (long)(((i) >> POS_A) & 0xFF)
#define GET_ARG_B(i) (long)(((i) >> POS_B) & 0xFF)
#define GET_ARG_C(i) (long)(((i) >> POS_C) & 0xFF)
#define GET_ARG_Ax(i) (long)(((i) >> POS_Ax) & 0xFFFFFF)
#define GET_ARG_Bx(i) (long)(((i) >> POS_Bx) & 0xFFFF)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // VM_OP_CODE_H
