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

#include "core/ecma/operations/ecma_regexp_object.h"
#include "core/vm/vm_ecma_value_helper.h"
#include "core/ecma/base/ecma_helpers.h"
#include "core/vm/vm_op_code.h"
#include "core/jmem/jmem.h"
#include "core/vm/vm_exec_state.h"
#include "core/ecma/base/ecma_globals.h"
#include "decoder/decoder_sections.h"
#include "decoder/exec_state_decode.h"
#include "base/common_logger.h"
#include "base/common_binary.h"
#include "core/ecma/operations/ecma_compiled_code.h"

#define RAISE_ERR_MSG(msg)\
  err = msg;\
  goto err_handle

#define CHECK_ERR(to_free)\
  if (qking_decoder_has_err(decoder)){\
  err = decoder->err_msg;\
  to_free;\
  goto err_handle;\
  }

#define RAISE_DECODER_ERR_MSG()\
  err = decoder->err_msg;\
  goto err_handle

#define FINISH_DECODE()\
  goto succ_handle

#define COMMON_START()\
  const char *err = NULL;\
  if (qking_decoder_fstream_tell(decoder->stream) < 0) {\
    RAISE_ERR_MSG("section is empty, eos");\
  }\

#define COMMON_RETURN()\
  succ_handle:\
  QKING_ASSERT(err == NULL);\
  return true;\
  err_handle:\
  QKING_ASSERT(err != NULL);\
  decoder->err_msg = err;\
  return false

static uint8_t gs_op_code_bits = 0;

typedef struct {
  uint8_t *data_;
  uint32_t byteOffset_;
  uint32_t currentValue_;
  uint32_t currentBits_;
  uint32_t size_;
} c_bits_reader_t;

static c_bits_reader_t *c_bits_reader_create(uint8_t *data, uint32_t size) {
  c_bits_reader_t *bits_reader = jmem_system_malloc(sizeof(c_bits_reader_t));
  bits_reader->data_ = data;
  bits_reader->size_ = size;
  bits_reader->byteOffset_ = 0;
  bits_reader->currentValue_ = 0;
  bits_reader->currentBits_ = 0;
  return bits_reader;
}

static uint32_t c_bit_remainingBytes(c_bits_reader_t *c_bits_reader) {
  return c_bits_reader->size_ - c_bits_reader->byteOffset_;
}

static uint32_t c_bit_remainingBits(c_bits_reader_t *c_bits_reader) {
  return 8 * (c_bits_reader->size_ - c_bits_reader->byteOffset_) + c_bits_reader->currentBits_;
}
static uint32_t c_bit_readStream(c_bits_reader_t *c_bits_reader, uint8_t *buffer, uint32_t size) {
  uint32_t bytes_reading = 0;
  do {
    if (size > c_bits_reader->size_ - c_bits_reader->byteOffset_) {
      break;
    }
    memcpy(buffer, c_bits_reader->data_ + c_bits_reader->byteOffset_, size);
    c_bits_reader->byteOffset_ += size;
    bytes_reading += size;

  } while (0);

  return bytes_reading;
}
static uint32_t c_bit_nextBits(c_bits_reader_t *c_bits_reader, uint32_t numBits) {
  assert(numBits <= c_bit_remainingBits(c_bits_reader) &&
      "attempt to read more bits than available");
  // Collect bytes until we have enough bits:
  while (c_bits_reader->currentBits_ < numBits) {
    c_bits_reader->currentValue_ =
        (c_bits_reader->currentValue_ << 8) + (uint32_t) (c_bits_reader->data_[c_bits_reader->byteOffset_]);
    c_bits_reader->currentBits_ = c_bits_reader->currentBits_ + 8;
    c_bits_reader->byteOffset_ = c_bits_reader->byteOffset_ + 1;
  }
  // Extract result:
  uint32_t remaining = c_bits_reader->currentBits_ - numBits;
  uint32_t result = c_bits_reader->currentValue_ >> remaining;
  // Update remaining bits:
  c_bits_reader->currentValue_ = c_bits_reader->currentValue_ & ((1 << remaining) - 1);
  c_bits_reader->currentBits_ = remaining;
  return result;
}

static void determine_op_code_bits() {
  if (gs_op_code_bits) {
    return;
  }

  gs_op_code_bits = 1;
  uint32_t op_code_value = OP_INVALID;
  while (op_code_value / 2 > 0) {
    op_code_value = op_code_value / 2;
    gs_op_code_bits++;
  }
}

static uint32_t SectionFunctionDecoder_decodingInstructionsFromBuffer(exec_state_decoder_t *decoder,
                                                                      exec_state_decoder_func_state_t *func_state,
                                                                      uint8_t *buffer,
                                                                      uint32_t bytes) {
  const char *err = NULL;
  c_bits_reader_t *reader = NULL;
  uint32_t bytes_reading = 0;
  do {
    reader = c_bits_reader_create(buffer, bytes);
    uint32_t count = 0;
    if (c_bit_readStream(reader, (uint8_t *) &count, sizeof(uint32_t)) != sizeof(uint32_t) || !count) {
      RAISE_ERR_MSG("decoding instructions buffer count zero error");
    }
    uint8_t half_bits_op_code = gs_op_code_bits / 2;
    ecma_compiled_func_state_create_instructions(func_state->func_state, count);
    for (int i = 0; i < count; i++) {
      uint8_t using_full_bits_op_code = c_bit_nextBits(reader, 1);
      uint32_t codeBits = c_bit_nextBits(reader, using_full_bits_op_code ? gs_op_code_bits : half_bits_op_code);
      if (codeBits >= OP_INVALID) {
        RAISE_ERR_MSG("decoding instructions code error");
      }
      uint32_t A = 0, B = 0, C = 0;
      uint8_t using8bits = 0, using24bits = 0;
      OPCode code = (OPCode) (codeBits);
      int ops = qking_oputil_get_ops(code);
      Instruction instruction;
      if (code == OP_GOTO || code == OP_TRY || code == OP_FINALLY) {
        using24bits = c_bit_nextBits(reader, 1);
        A = c_bit_nextBits(reader, using24bits ? 24 : 12);
        instruction = CREATE_Ax(code, A);
      } else if (code == OP_N_JMP || code == OP_JMP_PC || code == OP_CATCH ||
          code == OP_LOADK || code == OP_GET_GLOBAL ||
          code == OP_GET_LOCAL) {
        using8bits = c_bit_nextBits(reader, 1);
        A = c_bit_nextBits(reader, using8bits ? 8 : 4);
        using24bits = c_bit_nextBits(reader, 1);
        B = c_bit_nextBits(reader, using24bits ? 24 : 12);
        instruction = CREATE_ABx(code, A, B);
      } else {
        switch (ops) {
          case 1: {
            using8bits = c_bit_nextBits(reader, 1);
            A = c_bit_nextBits(reader, using8bits ? 8 : 4);
            instruction = CREATE_Ax(code, A);
            break;
          }
          case 2: {
            using8bits = c_bit_nextBits(reader, 1);
            A = c_bit_nextBits(reader, using8bits ? 8 : 4);
            using8bits = c_bit_nextBits(reader, 1);
            B = c_bit_nextBits(reader, using8bits ? 8 : 4);
            instruction = CREATE_ABx(code, A, B);
            break;
          }
          case 3: {
            using8bits = c_bit_nextBits(reader, 1);
            A = c_bit_nextBits(reader, using8bits ? 8 : 4);
            using8bits = c_bit_nextBits(reader, 1);
            B = c_bit_nextBits(reader, using8bits ? 8 : 4);
            using8bits = c_bit_nextBits(reader, 1);
            C = c_bit_nextBits(reader, using8bits ? 8 : 4);
            instruction = CREATE_ABC(code, A, B, C);
            break;
          }
          default:
            instruction = CREATE_Ax(code, 0);
            break;
        }
      }
      LOGD("decoding Instructions:%ld", instruction);
      (func_state->func_state->pc)[i] = instruction;
    }
    if (c_bit_remainingBytes(reader)) {
      RAISE_ERR_MSG("decoding instructions code error");
      break;
    }
    bytes_reading = bytes;

  } while (0);

  succ_handle:
  if (reader) {
    jmem_system_free(reader);
  }
  QKING_ASSERT(err == NULL);
  return bytes_reading;

  err_handle:
  if (reader) {
    jmem_system_free(reader);
  }
  QKING_ASSERT(err != NULL);
  decoder->err_msg = err;
  return 0;
}

static uint32_t DecodeSection_decodingFromBuffer(uint8_t *src_buffer,
                                                 uint32_t src_buffer_len,
                                                 uint8_t *buffer, uint32_t *len) {
  uint32_t bytes_reader = 0;
  do {
    if (!buffer || !len) {
      break;
    }
    if (*len > src_buffer_len) {
      break;
    }
    memcpy(buffer, src_buffer, *len);
    bytes_reader = *len;

  } while (0);

  return bytes_reader;
}

bool qking_decoder_section_decode_header(exec_state_decoder_t *decoder, uint32_t length) {
  COMMON_START();
  int64_t seek_end = qking_decoder_fstream_tell(decoder->stream) + length;

  uint32_t compartible_version = 0;
  uint32_t version = 0;

  do {
    uint16_t target = 0;
    uint32_t value_length = qking_decoder_fstream_read_target(decoder->stream, &target, NULL, NULL);
    if (!value_length) {
      RAISE_ERR_MSG("decoding header target error");
    }

    switch (target) {
      case kHeaderMagicCode: {
        uint32_t magic_code = 0;
        if (qking_decoder_fstream_read(decoder->stream, &magic_code, 1, value_length) != value_length ||
            magic_code != EXEC_BINARY_MAGIC_NUMBER) {
          RAISE_ERR_MSG("decoding header target magic code error");
        }
        break;
      }
      case kHeaderVersion: {
        if (qking_decoder_fstream_read(decoder->stream, &version, 1, value_length) != value_length) {
          RAISE_ERR_MSG("decoding header target version code error");
        }
        break;
      }
      case kHeaderCompartibleVersion: {
        if (qking_decoder_fstream_read(decoder->stream, &compartible_version, 1, value_length) !=
            value_length) {
          RAISE_ERR_MSG("decoding header target compartible version error");
        }
        if (EXEC_BINARY_CURRENT_VERSION < compartible_version) {
          RAISE_ERR_MSG("decoding header target current version didn't compartible");
        }
        break;
      }
      case kHeaderEncrypt: {
        uint32_t enc;
        if (qking_decoder_fstream_read(decoder->stream, &enc, 1, value_length) != value_length) {
          RAISE_ERR_MSG("decoding header target encrypt error");
        }
        break;
      }
      default: {  // skip this node
        if (qking_decoder_fstream_seek(decoder->stream, value_length, SEEK_CUR) < 0) {
          RAISE_ERR_MSG("err_skip node in header");
        }
        break;
      }
    }
    if (qking_decoder_fstream_tell(decoder->stream) < 0) {
      RAISE_ERR_MSG("qking_decoder_fstream_tell(decoder->stream) < 0");
    }

  } while (qking_decoder_fstream_tell(decoder->stream) < seek_end);
COMMON_RETURN();
}

uint32_t DecodeSection_decodingValueFromBuffer(exec_state_decoder_t *decoder, uint8_t *buffer,
                                               uint32_t remain, ecma_value_t *pval) {
  const char *err = NULL;

  bool is_regex = false;
  ecma_value_t flag = ECMA_VALUE_UNDEFINED;
  ecma_value_t str = ECMA_VALUE_UNDEFINED;

  uint32_t total_bytes_read = 0;
  do {
    uint32_t bytes_read = 0;
    uint32_t bytes_decoding = sizeof(uint8_t);
    int8_t type = 0;
    if (!(bytes_read = DecodeSection_decodingFromBuffer(buffer, remain, (uint8_t *) &type,
                                                        &bytes_decoding))) {
      RAISE_ERR_MSG("value bytes_read err");
    }
    total_bytes_read += bytes_read;
    buffer += bytes_read;
    remain -= bytes_read;
    switch (type) {
      case ENCODE_VALUE_TYPE_REGEX: {
        is_regex = true;

        uint32_t u32_payload = 0;
        bytes_decoding = sizeof(uint32_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &u32_payload, &bytes_decoding))) {
          RAISE_ERR_MSG("value regex bytes_read err");
        }
        if (u32_payload >= decoder->regex_flag_array_size) {
          RAISE_ERR_MSG("decoding value payload regex flag error");
        }
        flag = decoder->regex_flag_array[u32_payload];

#ifdef DEBUG_LOG_ENABLE
        QKING_TO_LOG_STR_START(flag_c_str, flag);
        LOGD("decoding regex flag string value:[%u] %s", u32_payload, flag_c_str);
        QKING_TO_LOG_STR_FINISH(flag_c_str);
#endif

        total_bytes_read += bytes_read;

        buffer += bytes_read;
        remain -= bytes_read;
/* fall through */
      }
      case ENCODE_VALUE_TYPE_STRING: {
        uint32_t u32_payload = 0;
        bytes_decoding = sizeof(uint32_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &u32_payload, &bytes_decoding))) {
          RAISE_ERR_MSG("value string bytes_read err");
        }
        if (u32_payload >= decoder->string_array_size) {
          RAISE_ERR_MSG("decoding value payload string error");
        }
        str = decoder->string_array[u32_payload];
        *pval = str;

#ifdef DEBUG_LOG_ENABLE
        QKING_TO_LOG_STR_START(flag_c_str, str);
        LOGD("decoding string value:[%u] %s", u32_payload, flag_c_str);
        QKING_TO_LOG_STR_FINISH(flag_c_str);
#endif

        total_bytes_read += bytes_read;
        break;
      }
      case ENCODE_VALUE_TYPE_REGEX_DIRECT: {
        is_regex = true;

        uint32_t u32_payload = 0;
        bytes_decoding = sizeof(uint32_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &u32_payload, &bytes_decoding))) {
          RAISE_ERR_MSG("value string bytes_read err");
        }
        if (u32_payload >= decoder->regex_flag_array_size) {
          RAISE_ERR_MSG("decoding value payload string error");
        }
        flag = decoder->regex_flag_array[u32_payload];

#ifdef DEBUG_LOG_ENABLE
        QKING_TO_LOG_STR_START(flag_c_str, flag);
        LOGD("decoding direct flag value:[%u] %s", u32_payload, flag_c_str);
        QKING_TO_LOG_STR_FINISH(flag_c_str);
#endif
        total_bytes_read += bytes_read;

        buffer += bytes_read;
        remain -= bytes_read;
/* fall through */
      }
      case ENCODE_VALUE_TYPE_STRING_DIRECT: {
        uint32_t u32_payload = 0;
        bytes_decoding = sizeof(uint32_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &u32_payload, &bytes_decoding))) {
          RAISE_ERR_MSG("value string-direct bytes_read err");
        }
        str = u32_payload;
        *pval = str;

#ifdef DEBUG_LOG_ENABLE
        QKING_TO_LOG_STR_START(flag_c_str, str);
        LOGD("decoding direct string value:[%u] %s", u32_payload, flag_c_str);
        QKING_TO_LOG_STR_FINISH(flag_c_str);
#endif
        total_bytes_read += bytes_read;
        break;
      }
      case ENCODE_VALUE_TYPE_INTEGER: {
        int32_t i32_payload = 0;
        bytes_decoding = sizeof(int32_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &i32_payload, &bytes_decoding))) {
          break;
        }
        *pval = ecma_make_int32_value(i32_payload);
        LOGD("decoding int value:%i", i32_payload);
        total_bytes_read += bytes_read;
        break;
      }
      case ENCODE_VALUE_TYPE_NUMBER: {
        double payload = 0;
        bytes_decoding = sizeof(double);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &payload, &bytes_decoding))) {
          break;
        }
        *pval = ecma_make_number_value(payload);
        LOGD("decoding number value:%lf", payload);
        total_bytes_read += bytes_read;
        break;
      }
      case ENCODE_VALUE_TYPE_BOOLEAN: {
        uint8_t payload;
        bytes_decoding = sizeof(uint8_t);
        if (!(bytes_read = DecodeSection_decodingFromBuffer(
            buffer, remain, (uint8_t *) &payload, &bytes_decoding))) {
          break;
        }
        bool result = !!(payload);
        *pval = result ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;
        LOGD("decoding bool value:%s", result ? "true" : "false");
        total_bytes_read += bytes_read;
        break;
      }
      default: {
        RAISE_ERR_MSG("decoding value type unkown error");
      }
    }

  } while (0);

  succ_handle:

  if (is_regex) {
    ecma_value_t regex_obj = ecma_op_create_regexp_object(ecma_get_string_from_value(str), ecma_get_string_from_value(flag));
    if (ECMA_IS_VALUE_ERROR(regex_obj)) {
      // regex parse err;
      RAISE_ERR_MSG("make regex obj err");
    }
    *pval = regex_obj;
  }

  QKING_ASSERT(err == NULL);
  return total_bytes_read;

  err_handle:

  QKING_ASSERT(err != NULL);
  decoder->err_msg = err;
  return 0;
}

bool qking_decoder_section_decode_string(exec_state_decoder_t *decoder, uint32_t length) {
  COMMON_START();

  //string array
  {
    uint16_t target = 0;
    uint32_t string_count = 0;
    uint32_t size = sizeof(uint32_t);
    uint32_t readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, (uint8_t *) &string_count, &size);
    if (readbytes != size || target != kValueStringSize) {
      RAISE_ERR_MSG("decoding string count error");
    }
    qking_decoder_create_string_table(decoder, string_count);
    int index = 0;
    for (index = 0; index < string_count; index++) {
      uint16_t vindex = 0;
      uint32_t varlen = qking_decoder_fstream_read_target(decoder->stream, &vindex, NULL, NULL);
      if (vindex != kValueStringPayload) {
        RAISE_ERR_MSG("vindex != kValueStringPayload");
      }
      if (varlen == 0) {
        decoder->string_array[index] = qking_create_string_from_utf8((const qking_char_t *) "");
        continue;
      }

      char *pstr = (char *) jmem_system_malloc(varlen + 1);
      if (!pstr) {
        RAISE_ERR_MSG("decode string malloc err");
      }
      memset(pstr, 0, varlen + 1);
      if (qking_decoder_fstream_read(decoder->stream, pstr, 1, varlen) != varlen) {
        RAISE_ERR_MSG("decoding string content error");
      }
      decoder->string_array[index] = qking_create_string_from_utf8((const qking_char_t *) pstr);
      LOGD("decoding string: %s", pstr);
      jmem_system_free(pstr);
    }
    if (index != string_count) {
      RAISE_ERR_MSG("decoding string count error");
    }
  }

  //flag decode
  {
    uint16_t target = 0;
    uint32_t string_count = 0;
    uint32_t size = sizeof(uint32_t);
    uint32_t readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, (uint8_t *) &string_count, &size);
    if (readbytes != size || target != kValueStringFlagSize) {
      RAISE_ERR_MSG("decoding flat count error");
    }
    qking_decoder_create_regex_flag_table(decoder, string_count);
    int index = 0;
    for (index = 0; index < string_count; index++) {
      uint16_t vindex = 0;
      uint32_t varlen = qking_decoder_fstream_read_target(decoder->stream, &vindex, NULL, NULL);
      if (vindex != kValueStringFlagPayload) {
        RAISE_ERR_MSG("vindex != kValueStringFlagPayload");
      }
      if (varlen == 0) {
        decoder->regex_flag_array[index] = qking_create_string_from_utf8((const qking_char_t *) "");
        continue;
      }

      char *pstr = (char *) jmem_system_malloc(varlen + 1);
      if (!pstr) {
        RAISE_ERR_MSG("decode flag malloc err");
      }
      memset(pstr, 0, varlen + 1);
      if (qking_decoder_fstream_read(decoder->stream, pstr, 1, varlen) != varlen) {
        RAISE_ERR_MSG("decoding flag content error");
      }
      decoder->regex_flag_array[index] = qking_create_string_from_utf8((const qking_char_t *) pstr);
      LOGD("decoding flag: %s", pstr);
      jmem_system_free(pstr);
    }
    if (index != string_count) {
      RAISE_ERR_MSG("decoding flag count error");
    }
  }

COMMON_RETURN();
}

bool qking_decoder_section_decode_function(exec_state_decoder_t *decoder, uint32_t length) {
  determine_op_code_bits();
  COMMON_START();

  int64_t seek_end = qking_decoder_fstream_tell(decoder->stream) + length;
  uint16_t target = 0;
  uint32_t func_count = 0;
  uint32_t size = sizeof(uint32_t);
  uint32_t readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, (uint8_t *) &func_count, &size);
  if (!readbytes || target != kValueFunctionSize || !func_count) {
    RAISE_ERR_MSG("!readbytes || target != kValueFunctionSize || !func_count");
  }
  qking_decoder_create_func_table(decoder, func_count);
  for (int func_index = 0; func_index < func_count; func_index++) {
    exec_state_decoder_func_state_t *decode_func = &decoder->func_state_p_array[func_index];

    uint32_t constants_count = 0;
    do {
      if ((readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, NULL, NULL)) == 0) {
        RAISE_ERR_MSG("decoding read function target error");
      }

      switch (target) {
        case kValueFunctionSuper: {
          int super_index = -1;
          if (qking_decoder_fstream_read(decoder->stream, &super_index, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding read super function error");
          }
          decode_func->super_index = super_index;
          break;
        }
        case kValueFunctionArgc: {
          uint32_t argc = 0;
          if (qking_decoder_fstream_read(decoder->stream, &argc, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding function argc error");
          }
          decode_func->func_state->argc = (uint8_t) argc;
          break;
        }
        case kValueFunctionStackSize: {
          uint32_t stack_size = 0;
          if (qking_decoder_fstream_read(decoder->stream, &stack_size, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding function stack_size error");
          }
          decode_func->func_state->stack_size = (uint16_t) stack_size;
          break;
        }
        case kValueFunctionContextDepth: {
          uint32_t context_depth = 0;
          if (qking_decoder_fstream_read(decoder->stream, &context_depth, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding function context_depth error");
          }
          decode_func->func_state->context_size = (uint16_t) context_depth;
          break;
        }
        case kValueFunctionStatusFlag: {
          uint16_t status_flag = 0;
          if (qking_decoder_fstream_read(decoder->stream, &status_flag, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding function status_flag error");
          }
          decode_func->func_state->header.status_flags = status_flag;
          break;
        }
        case kValueFunctionInClosure: {
          int32_t *buffer = (int32_t *) jmem_system_malloc(readbytes);
          if (!buffer) {
            RAISE_ERR_MSG("decoding low memory error");
          }
          if (qking_decoder_fstream_read(decoder->stream, buffer, 1, readbytes) != readbytes) {
            jmem_system_free(buffer);
            RAISE_ERR_MSG("decoding function in closure error");
          }
          int32_t in_size = readbytes / (2 * sizeof(int32_t));
          qking_decoder_create_in_closure_ref_table(decode_func, (size_t) in_size);
          for (int i = 0; i < in_size; i++) {
            decode_func->in_closure_refs[i] = buffer[i * 2];
            decode_func->in_closure_ids[i] = buffer[i * 2 + 1];
            LOGD("decoding function in closure: ref: %d, id: %d", buffer[i * 2],buffer[i * 2 + 1]);
          }
          jmem_system_free(buffer);
          break;
        }
        case kValueFunctionOutClosure: {
          int32_t *buffer = (int32_t *) jmem_system_malloc(readbytes);
          if (!buffer) {
            RAISE_ERR_MSG("decoding low memory error");
          }
          if (qking_decoder_fstream_read(decoder->stream, buffer, 1, readbytes) != readbytes) {
            jmem_system_free(buffer);
            RAISE_ERR_MSG("decoding function out closure error");
          }
          int32_t out_size = readbytes / sizeof(int32_t);
          qking_decoder_create_out_closure_ref_table(decode_func, (size_t) out_size);
          for (int i = 0; i < out_size; i++) {
            decode_func->out_closure_refs[i] = buffer[i];
            LOGD("decoding function out closure:%i", buffer[i]);
          }
          jmem_system_free(buffer);
          break;
        }
        case kValueFunctionInstructions: {
          uint8_t *buffer = (uint8_t *) jmem_system_malloc(readbytes);
          if (!buffer) {
            RAISE_ERR_MSG("decoding read function low memory error");
          }
          if (qking_decoder_fstream_read(decoder->stream, buffer, 1, readbytes) != readbytes) {
            jmem_system_free(buffer);
            RAISE_ERR_MSG("decoding read function instructions error");
          }
          bool read_same =
              SectionFunctionDecoder_decodingInstructionsFromBuffer(decoder, decode_func, buffer, readbytes)
                  != readbytes;
          CHECK_ERR(jmem_system_free(buffer));
          if (read_same) {
            jmem_system_free(buffer);
            RAISE_ERR_MSG("decoding read function instructions error");
          }
          jmem_system_free(buffer);
          break;
        }
        case kValueFunctionConstantCount: {
          if (qking_decoder_fstream_read(decoder->stream, &constants_count, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding read function constants count error");
          }
          break;
        }
        case kValueFunctionConstantPayload: {
          if (!constants_count) {
            RAISE_ERR_MSG("decoding read function constants count error");
          }
          uint8_t *buffer = (uint8_t *) jmem_system_malloc(readbytes);
          if (!buffer) {
            RAISE_ERR_MSG("low memory error");
          }
          if (qking_decoder_fstream_read(decoder->stream, buffer, 1, readbytes) != readbytes) {
            jmem_system_free(buffer);
            RAISE_ERR_MSG("decoding function constants payload error");
          }
          uint8_t *read_buffer = buffer;
          uint32_t remain_length = readbytes;
          uint32_t bytes_read = 0;
          ecma_compiled_func_state_create_constants(decode_func->func_state, constants_count);
          for (int i = 0; i < constants_count; i++) {
            ecma_value_t value;
            bool empty_read =
                !(bytes_read = DecodeSection_decodingValueFromBuffer(decoder, read_buffer, remain_length, &value));
            CHECK_ERR(jmem_system_free(buffer));
            if (empty_read) {
              jmem_system_free(buffer);
              RAISE_ERR_MSG("decoding function constants payload error");
            }
            read_buffer += bytes_read;
            remain_length -= bytes_read;

            ecma_value_t ret = qking_set_property_by_index(
                decode_func->func_state->constants, (uint32_t) (i), value);
#ifndef QKING_NDEBUG
            if (ecma_is_value_number(value) || ecma_is_value_object(value)) {
                qking_release_value(value);
            }
#endif
            bool is_err = ecma_is_value_error_reference(ret);
            if (is_err) {
              ret = qking_get_value_from_error(ret, true);
              QKING_TO_LOG_STR_START(flag_c_str, ret);
              LOGE("Set Constant[%d] Expception %s", i, flag_c_str);
              QKING_TO_LOG_STR_FINISH(flag_c_str);
              qking_release_value(ret);
              jmem_system_free(buffer);
              RAISE_ERR_MSG("ConvertToCompiledFunction set constant err");
            }
          }
          jmem_system_free(buffer);
          break;
        }
        case kValueFunctionFinished: {
          uint8_t function_finshed = 0;
          if (qking_decoder_fstream_read(decoder->stream, &function_finshed, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding function finished error");
          }
          if (function_finshed != SECTION_VALUE_FINISHED) {
            RAISE_ERR_MSG("decoding function finished error");
          }
          break;
        }
        default: {
          if (qking_decoder_fstream_seek(decoder->stream, readbytes, SEEK_CUR) < 0) {
            RAISE_ERR_MSG("function decoding length error");
          }
          break;
        }
      }
      if (target == SECTION_VALUE_FINISHED) {
#ifndef QKING_NDEBUG
        ecma_compiled_function_state_t *p_func_state = decode_func->func_state;
        LOGD("# argc: %u, call_stack: %u, context_depth %u",
             (uint32_t)(p_func_state->argc), p_func_state->stack_size,
             p_func_state->context_size);
        LOGD("# in_closure count: %u, out_closure count: %u",
             decode_func->in_closure_refs_count, decode_func->out_closure_refs_count);
#endif
        break;
      }

    } while (qking_decoder_fstream_tell(decoder->stream) < seek_end);
  }

  if (!func_count) {
    RAISE_ERR_MSG("function decoding no funcState?");
  }
  decoder->exec_state->compiled_func_state = decoder->func_state_p_array[0].func_state;
  COMMON_RETURN();
}

bool qking_decoder_section_decode_value_ref(exec_state_decoder_t *decoder, uint32_t length) {
  COMMON_START();

  uint16_t target = 0;
  uint32_t refs_size = 0;
  uint32_t size = sizeof(uint32_t);
  uint32_t readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, (uint8_t *) &refs_size, &size);
  if (!readbytes || target != kValueOutRefSize || !refs_size) {
    RAISE_ERR_MSG("!readbytes || target != kValueRefSize || !refs_size");
  }
  int64_t seek_end = qking_decoder_fstream_tell(decoder->stream) + length;

  // Convert qking_context(exec_state) level;
  {
    if (refs_size > 0) {
      ecma_func_out_closure_t **func_out_closure_pp =
          (ecma_func_out_closure_t **) jmem_heap_alloc_block(
              sizeof(ecma_func_out_closure_t *) * refs_size);
      QKING_CONTEXT(vm_func_out_closure_pp) = func_out_closure_pp;
      LOGD("Exec State refs count: %d", size);
      QKING_CONTEXT(vm_func_out_closure) = (uint32_t)refs_size;
    }
  }

  for (uint32_t i = 0; i < refs_size; i++) {
    ecma_func_out_closure_t *func_closure_info_p = (ecma_func_out_closure_t *) jmem_heap_alloc_block(sizeof(ecma_func_out_closure_t));
    do {
      if ((readbytes = qking_decoder_fstream_read_target(decoder->stream, &target, NULL, NULL)) == 0) {
        RAISE_ERR_MSG("decoding value ref target error");
      }
      switch (target) {
        case kValueOutRefFuncStateIndex: {
          uint32_t buffer;
          if (qking_decoder_fstream_read(decoder->stream, &buffer, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding value ref function state error");
          }
          if (buffer >= decoder->func_state_p_array_size) {
            RAISE_ERR_MSG("decoding value ref function state index overflow");
          }
          func_closure_info_p->func_state_p = decoder->func_state_p_array[buffer].func_state;
          break;
        }
        case kValueOutRefIdx: {
          int32_t register_id_reader = -1;
          if (qking_decoder_fstream_read(decoder->stream, &register_id_reader, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding value register id error");
            break;
          }
          if (register_id_reader < 0) {
            RAISE_ERR_MSG("decoding value register id error");
          }
          func_closure_info_p->idx = register_id_reader;
          break;
        }
        case kValueOutRefFinished: {
          uint8_t finished = 0;
          if (qking_decoder_fstream_read(decoder->stream, &finished, 1, readbytes) != readbytes) {
            RAISE_ERR_MSG("decoding value ref finished error");
            break;
          }
          if (finished != SECTION_VALUE_FINISHED) {
            RAISE_ERR_MSG("decoding value ref finished error");
            break;
          }
          break;
        }
        default: {
          if (qking_decoder_fstream_seek(decoder->stream, readbytes, SEEK_CUR) < 0) {
            RAISE_ERR_MSG("decoding value ref length error");
          }
          break;
        }
      }
      if (target == kValueOutRefFinished) {
        break;
      }

    } while (qking_decoder_fstream_tell(decoder->stream) < seek_end);
    QKING_CONTEXT(vm_func_out_closure_pp)[i] = func_closure_info_p;
  }
COMMON_RETURN();
}

