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

#include <string.h>
#include "core/ecma/operations/ecma_compiled_code.h"
#include "decoder/exec_state_decode.h"
#include "decoder/decoder_sections.h"
#include "base/common_binary.h"
#include "base/common_logger.h"
#include "exec_state_decode.h"
#ifdef QKING_ENABLE_GC_DEBUG
#include "core/ecma/base/ecma_gc.h"
#endif

int64_t qking_decoder_fstream_seek(f_stream_t *stream, int64_t pos, int type) {
  switch (type) {
    case SEEK_SET: {
      stream->seek = pos > stream->size ? stream->size : pos;
      break;
    }
    case SEEK_CUR: {
      if (stream->seek + pos > stream->size) {
        stream->seek = stream->size;
      } else {
        stream->seek += pos;
      }
      break;
    }
    case SEEK_END: {
      if (pos >= 0) {
        stream->seek = stream->size;
      } else {
        if (stream->seek <= (uint64_t) (-pos)) {
          stream->seek = 0;
        } else {
          stream->seek -= (uint64_t) (-pos);
        }
      }
      break;
    }
  }
  return (int64_t) stream->seek;
}

static uint32_t qking_decoder_fstream_read_internal(f_stream_t *stream, void *buffer, uint32_t size) {
  uint32_t readBytes = 0;
  do {
    int64_t left = stream->size - stream->seek;
    if (left < 0) {
      break;
    }
    if (left < (int64_t) size) {
      size = (uint32_t) left;
    }
    memcpy(buffer, stream->buffer + stream->seek, size);
    stream->seek += size;
    readBytes = size;

  } while (0);

  return readBytes;
}

uint32_t qking_decoder_fstream_read(f_stream_t *stream, void *buffer, uint32_t size, uint32_t cnt) {
  return qking_decoder_fstream_read_internal(stream, buffer, size * cnt) / size;
}
uint32_t qking_decoder_fstream_read_target(f_stream_t *stream, uint16_t *target, uint8_t *buffer, uint32_t *size) {
  uint32_t bytesRead = 0;
  do {
    uint32_t flags = 0;
    uint32_t target_length = 0;
    uint16_t target_value = 0;
    uint32_t size_length = 0;
    uint32_t value_length = 0;
    if (qking_decoder_fstream_read(stream, &flags, 1, 1) != 1) {
      break;
    }
    if (SECTION_TEST_FLAG(flags, SECTION_EXT_MASK)) {
      if (qking_decoder_fstream_read(stream, (uint8_t *) &flags + 1, 1, 3) != 3) {
        break;
      }
    }
    // tag
    target_length = SECTION_TEST_FLAG(flags, SECTION_TAG_MASK) ? 2 : 1;
    if (qking_decoder_fstream_read(stream, &target_value, 1, target_length) != target_length) {
      break;
    }
    // length
    SECTION_GET_LENGTH_LEN(flags, size_length);
    if (size_length == 0) {
      value_length = (flags & SECTION_VALUE_MASK) + 1;  // 0:1 1:2 2:4 3:8
      if (value_length == 3) {
        value_length = 4;
      } else if (value_length == 4) {
        value_length = 8;
      }
    } else {
      if (qking_decoder_fstream_read(stream, &value_length, 1, size_length) != size_length) {
        break;
      }
    }
    if (target) {
      *target = target_value;
    }
    if (buffer == NULL || size == NULL || *size == 0 || value_length == 0) {
      bytesRead = value_length;
      break;
    }
    uint32_t buffer_length = *size;
    *size = value_length;
    if (buffer_length > value_length) {
      buffer_length = value_length;
    }
    if (qking_decoder_fstream_read(stream, buffer, 1, buffer_length) != buffer_length) {
      break;
    }
    if (buffer_length < value_length) {
      qking_decoder_fstream_seek(stream, value_length - buffer_length, SEEK_CUR);
    }
    bytesRead = value_length;

  } while (0);

  return bytesRead;
}

inline int64_t qking_decoder_fstream_tell(f_stream_t *stream) {
  return stream->seek;
}

exec_state_decoder_t *qking_create_decoder(qking_vm_exec_state_t *exec_state, uint8_t *buffer, size_t size) {
  exec_state_decoder_t *result = (exec_state_decoder_t *) jmem_system_malloc(sizeof(exec_state_decoder_t));
  result->exec_state = exec_state;

  result->func_state_p_array = NULL;
  result->func_state_p_array_size = 0;

  result->string_array = NULL;
  result->string_array_size = 0;

  result->regex_flag_array = NULL;
  result->regex_flag_array_size = 0;

  result->err_msg = NULL;

  result->stream = (f_stream_t *) jmem_system_malloc(sizeof(f_stream_t));
  result->stream->buffer = buffer;
  result->stream->size = size;
  result->stream->seek = 0;
  return result;
}

inline void qking_decoder_raise_err(exec_state_decoder_t *decoder, const char *err) {
  decoder->err_msg = err;
}

inline bool qking_decoder_has_err(exec_state_decoder_t *decoder) {
  return decoder->err_msg != NULL;
}

void qking_free_decoder(exec_state_decoder_t *decoder) {
  jmem_system_free(decoder->stream);
  if (decoder->func_state_p_array) {
    for (size_t i = 0; i < decoder->func_state_p_array_size; ++i) {
      if(decoder->func_state_p_array[i].out_closure_refs){
        jmem_system_free(decoder->func_state_p_array[i].out_closure_refs);
      }

      if(decoder->func_state_p_array[i].in_closure_refs){
        jmem_system_free(decoder->func_state_p_array[i].in_closure_refs);
      }

      if(decoder->func_state_p_array[i].in_closure_ids){
        jmem_system_free(decoder->func_state_p_array[i].in_closure_ids);
      }
    }

    jmem_system_free(decoder->func_state_p_array);
  }
  if (decoder->string_array) {
    jmem_system_free(decoder->string_array);
  }
  if (decoder->regex_flag_array) {
    jmem_system_free(decoder->regex_flag_array);
  }
  jmem_system_free(decoder);
}

bool qking_decoding_binary(exec_state_decoder_t *decoder, const char **err_str) {
#define RAISE_ERR_MSG(msg)\
  err = msg;\
  goto err_handle

#define RAISE_DECODER_ERR_MSG()\
  err = decoder->err_msg;\
  goto err_handle
    
  const char *err = NULL;

#ifdef QKING_ENABLE_GC_DEBUG
    ecma_gc_info_ignore_start();
#endif

  if (qking_decoder_fstream_seek(decoder->stream, 0, SEEK_END) <= 0) {
    RAISE_ERR_MSG("file length zero error");
  }
  int64_t fileSize = qking_decoder_fstream_tell(decoder->stream);
  if (qking_decoder_fstream_seek(decoder->stream, 0, SEEK_SET) < 0) {
    RAISE_ERR_MSG("file length zero error");
  }
  do {
    uint16_t section = 0;
    uint32_t section_length = 0;
    if ((section_length = qking_decoder_fstream_read_target(decoder->stream, &section, NULL, NULL)) == 0) {
      RAISE_ERR_MSG("read section flag error");
    }
    switch (section) {
      case EXEC_SECTION_HEADER: {
        if (!qking_decoder_section_decode_header(decoder, section_length)) {
          RAISE_DECODER_ERR_MSG();
        }
        LOGD("[Decode section] SectionHeaderDecoder, size: %u\n",
             section_length);
        break;
      }
      case EXEC_SECTION_STRING: {
        if (!qking_decoder_section_decode_string(decoder, section_length)) {
          RAISE_DECODER_ERR_MSG();
        }
        LOGD("[Decode section] SectionStringDecoder, size: %u\n",
             section_length);
        break;
      }
      case EXEC_SECTION_FUNCTION: {
        if (!qking_decoder_section_decode_function(decoder, section_length)) {
          RAISE_DECODER_ERR_MSG();
        }
        LOGD("[Decode section] SectionFunctionDecoder, size: %u\n",
             section_length);
        break;
      }
      case EXEC_SECTION_VALUEREF: {
        if (!qking_decoder_section_decode_value_ref(decoder, section_length)) {
          RAISE_DECODER_ERR_MSG();
        }
        LOGD("[Decode section] SectionValueRefDecoder, size: %u\n",
             section_length);
        break;
      }
      default: {
        if (qking_decoder_fstream_seek(decoder->stream, section_length, SEEK_CUR) < 0) {
          RAISE_ERR_MSG("section decoding length error");
        }
        break;
      }
    }

  } while (qking_decoder_fstream_tell(decoder->stream) < fileSize);

  //calculate child count;
  for (size_t i = 0; i < decoder->func_state_p_array_size; ++i) {
    int32_t super = decoder->func_state_p_array[i].super_index;
    if (super<0){
      continue;
    }
    
    decoder->func_state_p_array[super].child_count++;
  }
  
  //allocate child
  for (size_t i = 0; i < decoder->func_state_p_array_size; ++i) {
    exec_state_decoder_func_state_t *ptr = decoder->func_state_p_array+i;
    ecma_compiled_func_state_create_children_array(ptr->func_state, ptr->child_count);
  }

  //link children
  for (size_t i = 0; i < decoder->func_state_p_array_size; ++i) {
    exec_state_decoder_func_state_t *this_func = decoder->func_state_p_array+i;

    int32_t super = this_func->super_index;
    if (super<0){
      continue;
    }

    exec_state_decoder_func_state_t *ptr = decoder->func_state_p_array+super;
    ptr->func_state->pp_childrens[ptr->current_child_end] = this_func->func_state;
    ptr->current_child_end++;
  }

  //link value-ref
  for (size_t i = 0; i < decoder->func_state_p_array_size; ++i) {
    exec_state_decoder_func_state_t *this_func = decoder->func_state_p_array + i;

    ecma_compiled_func_state_create_in_closure(this_func->func_state, this_func->in_closure_refs_count);
    for (size_t j = 0; j < this_func->in_closure_refs_count; ++j) {
      ecma_func_in_closure_t *func_in_closure_p =
          (ecma_func_in_closure_t *)jmem_heap_alloc_block(
              sizeof(ecma_func_in_closure_t));
      this_func->func_state->pp_in_closure[j] = func_in_closure_p;
      func_in_closure_p->out_closure_p = QKING_CONTEXT(vm_func_out_closure_pp)[this_func->in_closure_refs[j]];
      func_in_closure_p->idx = this_func->in_closure_ids[j];
    }

    ecma_compiled_func_state_create_out_closure(this_func->func_state, this_func->out_closure_refs_count);
    for (size_t j = 0; j < this_func->out_closure_refs_count; ++j) {
      this_func->func_state->pp_out_closure[j] = QKING_CONTEXT(vm_func_out_closure_pp)[this_func->out_closure_refs[j]];
    }
  }
    
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_info_ignore_end();
#endif

  return true;
  err_handle:
    
#ifdef QKING_ENABLE_GC_DEBUG
  ecma_gc_info_ignore_end();
#endif

  QKING_ASSERT(err != NULL);
  *err_str = err;
  return false;

#undef RAISE_ERR_MSG
#undef RAISE_DECODER_ERR_MSG
}

void qking_decoder_create_string_table(exec_state_decoder_t *decoder, size_t size) {
  size_t alloc_size = size;
  if (size == 0) {
    alloc_size += 1;//avoid empty alloc
  }

  decoder->string_array = (c_str_t *) jmem_system_malloc(sizeof(c_str_t) * alloc_size);
  decoder->string_array_size = size;
}

void qking_decoder_create_regex_flag_table(exec_state_decoder_t *decoder, size_t size) {
  size_t alloc_size = size;
  if (size == 0) {
    alloc_size += 1;//avoid empty alloc
  }

  decoder->regex_flag_array = (c_str_t *) jmem_system_malloc(sizeof(c_str_t) * alloc_size);
  decoder->regex_flag_array_size = size;
}

void qking_decoder_create_func_table(exec_state_decoder_t *decoder, size_t size) {
  size_t alloc_size = size;
  if (size == 0) {
    alloc_size += 1;//avoid empty alloc
  }

  decoder->func_state_p_array =
      (exec_state_decoder_func_state_t *) jmem_system_malloc(sizeof(exec_state_decoder_func_state_t) * alloc_size);
  decoder->func_state_p_array_size = size;

  for (size_t i = 0; i < size; ++i) {
    exec_state_decoder_func_state_t *func = &decoder->func_state_p_array[i];
    func->func_state = ecma_create_compiled_function_state();
    func->super_index = -1;
    func->child_count = 0;
    func->current_child_end = 0;

    func->in_closure_refs = NULL;
    func->in_closure_ids = NULL;
    func->in_closure_refs_count = 0;

    func->out_closure_refs = NULL;
    func->out_closure_refs_count = 0;
  }
}

void qking_decoder_create_in_closure_ref_table(exec_state_decoder_func_state_t *func,
                                               size_t size) {
  size_t alloc_size = size;
  if (size == 0) {
    alloc_size += 1;//avoid empty alloc
  }

  func->in_closure_refs = (int32_t *) jmem_system_malloc(sizeof(int32_t) * alloc_size);
  func->in_closure_ids = (int32_t *) jmem_system_malloc(sizeof(int32_t) * alloc_size);
  func->in_closure_refs_count = size;
}

void qking_decoder_create_out_closure_ref_table(exec_state_decoder_func_state_t *func,
                                                size_t size) {
  size_t alloc_size = size;
  if (size == 0) {
    alloc_size += 1;//avoid empty alloc
  }

  func->out_closure_refs = (int32_t *) jmem_system_malloc(sizeof(int32_t) * alloc_size);
  func->out_closure_refs_count = size;
}
