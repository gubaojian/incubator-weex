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
#ifndef QKING_SNAPSHOT_H
#define QKING_SNAPSHOT_H

#include "ecma_globals.h"

/**
 * Snapshot header
 */
typedef struct {
  /* The size of this structure is recommended to be divisible by
   * uint32_t alignment. Otherwise some bytes after the header are wasted. */
  uint32_t magic;            /**< four byte magic number */
  uint32_t version;          /**< version number */
  uint32_t global_flags;     /**< global configuration and feature flags */
  uint32_t lit_table_offset; /**< byte offset of the literal table */
  uint32_t number_of_funcs;  /**< number of primary ECMAScript functions */
  uint32_t func_offsets[1];  /**< function offsets (lowest bit: global(0) or
                                eval(1) context) */
} qking_snapshot_header_t;

/**
 * Qking snapshot magic marker.
 */
#define QKING_SNAPSHOT_MAGIC (0x5952524Au)

/**
 * Snapshot configuration flags.
 */
typedef enum {
  /* 8 bits are reserved for dynamic features */
  QKING_SNAPSHOT_HAS_REGEX_LITERAL =
      (1u << 0), /**< byte code has regex literal */
  QKING_SNAPSHOT_HAS_CLASS_LITERAL =
      (1u << 1), /**< byte code has class literal */
  /* 24 bits are reserved for compile time features */
  QKING_SNAPSHOT_FOUR_BYTE_CPOINTER =
      (1u << 8) /**< deprecated, an unused placeholder now */
} qking_snapshot_global_flags_t;

#endif /* !QKING_SNAPSHOT_H */
