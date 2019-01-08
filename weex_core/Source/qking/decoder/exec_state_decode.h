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

#ifndef QKING_ROOT_EXEC_STATE_DECODE_H
#define QKING_ROOT_EXEC_STATE_DECODE_H

#include "core/vm/vm_exec_state.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  int64_t seek;
  size_t size;
  uint8_t* buffer;
} f_stream_t;

int64_t qking_decoder_fstream_seek(f_stream_t* stream, int64_t pos, int type);

uint32_t qking_decoder_fstream_read(f_stream_t* stream, void* buffer,
                                    uint32_t size, uint32_t cnt);

uint32_t qking_decoder_fstream_read_target(f_stream_t* stream, uint16_t* target,
                                           uint8_t* buffer, uint32_t* size);

int64_t qking_decoder_fstream_tell(f_stream_t* stream);

typedef ecma_value_t c_str_t;

typedef struct {
  ecma_compiled_function_state_t* func_state;
  int32_t super_index;
  size_t child_count;
  size_t current_child_end;

  int32_t* in_closure_refs;
  int32_t* in_closure_ids;
  size_t in_closure_refs_count;

  int32_t* out_closure_refs;
  size_t out_closure_refs_count;
} exec_state_decoder_func_state_t;

typedef struct {
  f_stream_t* stream;
  qking_vm_exec_state_t* exec_state;

  exec_state_decoder_func_state_t* func_state_p_array;
  size_t func_state_p_array_size;

  c_str_t* string_array;
  size_t string_array_size;

  c_str_t* regex_flag_array;
  size_t regex_flag_array_size;

  const char* err_msg;
} exec_state_decoder_t;

exec_state_decoder_t* qking_create_decoder(qking_vm_exec_state_t* exec_state,
                                           uint8_t* buffer, size_t size);

void qking_free_decoder(exec_state_decoder_t* decoder);

bool qking_decoding_binary(exec_state_decoder_t* decoder, const char** err_str);

void qking_decoder_create_string_table(exec_state_decoder_t* decoder,
                                       size_t size);

void qking_decoder_create_regex_flag_table(exec_state_decoder_t* decoder,
                                           size_t size);

void qking_decoder_create_func_table(exec_state_decoder_t* decoder,
                                     size_t size);

void qking_decoder_create_in_closure_ref_table(
    exec_state_decoder_func_state_t* func, size_t size);

void qking_decoder_create_out_closure_ref_table(
    exec_state_decoder_func_state_t* func, size_t size);

void qking_decoder_raise_err(exec_state_decoder_t* decoder, const char* err);

bool qking_decoder_has_err(exec_state_decoder_t* decoder);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // QKING_ROOT_EXEC_STATE_DECODE_H
