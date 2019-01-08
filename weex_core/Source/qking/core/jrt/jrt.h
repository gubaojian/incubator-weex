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
#ifndef JRT_H
#define JRT_H

#include <stdio.h>
#include <string.h>

#include "jrt_types.h"
#include "qking_port.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Normally compilers store const(ant)s in ROM. Thus saving RAM.
 * But if your compiler does not support it then the directive below can force
 * it.
 *
 * For the moment it is mainly meant for the following targets:
 *      - ESP8266
 */
#ifndef QKING_CONST_DATA
#define QKING_CONST_DATA
#endif /* QKING_CONST_DATA */

/*
 * Constants
 */
#define QKING_BITSINBYTE 8

/*
 * Make sure unused parameters, variables, or expressions trigger no compiler
 * warning.
 */
#define QKING_UNUSED(x) ((void)(x))

#define QKING_UNUSED_1(_1) QKING_UNUSED(_1)
#define QKING_UNUSED_2(_1, _2) QKING_UNUSED(_1), QKING_UNUSED_1(_2)
#define QKING_UNUSED_3(_1, _2, _3) QKING_UNUSED(_1), QKING_UNUSED_2(_2, _3)
#define QKING_UNUSED_4(_1, _2, _3, _4) \
  QKING_UNUSED(_1), QKING_UNUSED_3(_2, _3, _4)
#define QKING_UNUSED_5(_1, _2, _3, _4, _5) \
  QKING_UNUSED(_1), QKING_UNUSED_4(_2, _3, _4, _5)

#define QKING_VA_ARGS_NUM_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define QKING_VA_ARGS_NUM(...) \
  QKING_VA_ARGS_NUM_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#define QKING_UNUSED_ALL_IMPL_(nargs) QKING_UNUSED_##nargs
#define QKING_UNUSED_ALL_IMPL(nargs) QKING_UNUSED_ALL_IMPL_(nargs)
#define QKING_UNUSED_ALL(...) \
  QKING_UNUSED_ALL_IMPL(QKING_VA_ARGS_NUM(__VA_ARGS__))(__VA_ARGS__)

/*
 * Asserts
 *
 * Warning:
 *         Don't use QKING_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define QKING_STATIC_ASSERT_GLUE_(a, b, c) a##b##_##c
#define QKING_STATIC_ASSERT_GLUE(a, b, c) QKING_STATIC_ASSERT_GLUE_(a, b, c)
#define QKING_STATIC_ASSERT(x, msg)                                     \
  enum {                                                                \
    QKING_STATIC_ASSERT_GLUE(static_assertion_failed_, __LINE__, msg) = \
        1 / (!!(x))                                                     \
  }

void QKING_ATTR_NORETURN qking_assert_fail(const char *assertion,
                                           const char *file,
                                           const char *function,
                                           const uint32_t line);

#define QKING_ASSERT(x)                                    \
  do {                                                     \
    if (QKING_UNLIKELY(!(x))) {                            \
      qking_assert_fail(#x, __FILE__, __func__, __LINE__); \
    }                                                      \
  } while (0)

#ifndef QKING_NDEBUG

void QKING_ATTR_NORETURN qking_unreachable(const char *file,
                                           const char *function,
                                           const uint32_t line);

#define QKING_UNREACHABLE()                          \
  do {                                               \
    qking_unreachable(__FILE__, __func__, __LINE__); \
  } while (0)
#else /* QKING_NDEBUG */

#ifdef __GNUC__
#define QKING_UNREACHABLE() __builtin_unreachable()
#endif /* __GNUC__ */

#ifdef _MSC_VER
#define QKING_UNREACHABLE() _assume(0)
#endif /* _MSC_VER */

#ifndef QKING_UNREACHABLE
#define QKING_UNREACHABLE()
#endif /* !QKING_UNREACHABLE */

#endif /* !QKING_NDEBUG */

/**
 * Exit on fatal error
 */
void QKING_ATTR_NORETURN qking_fatal(qking_fatal_code_t code);

/*
 * Logging
 */
#ifdef QKING_ENABLE_LOGGING
#define QKING_ERROR_MSG(...) qking_port_log(QKING_LOG_LEVEL_ERROR, __VA_ARGS__)
#define QKING_WARNING_MSG(...) \
  qking_port_log(QKING_LOG_LEVEL_WARNING, __VA_ARGS__)
#define QKING_DEBUG_MSG(...) qking_port_log(QKING_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define QKING_TRACE_MSG(...) qking_port_log(QKING_LOG_LEVEL_TRACE, __VA_ARGS__)
#else /* !QKING_ENABLE_LOGGING */
#define QKING_ERROR_MSG(...)         \
  do {                               \
    if (false) {                     \
      QKING_UNUSED_ALL(__VA_ARGS__); \
    }                                \
  } while (0)
#define QKING_WARNING_MSG(...)       \
  do {                               \
    if (false) {                     \
      QKING_UNUSED_ALL(__VA_ARGS__); \
    }                                \
  } while (0)
#define QKING_DEBUG_MSG(...)         \
  do {                               \
    if (false) {                     \
      QKING_UNUSED_ALL(__VA_ARGS__); \
    }                                \
  } while (0)
#define QKING_TRACE_MSG(...)         \
  do {                               \
    if (false) {                     \
      QKING_UNUSED_ALL(__VA_ARGS__); \
    }                                \
  } while (0)
#endif /* QKING_ENABLE_LOGGING */

/**
 * Size of struct member
 */
#define QKING_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) \
  sizeof(((struct_name *)NULL)->member_name)

/**
 * Aligns @a value to @a alignment. @a must be the power of 2.
 *
 * Returns minimum positive value, that divides @a alignment and is more than or
 * equal to @a value
 */
#define QKING_ALIGNUP(value, alignment) \
  (((value) + ((alignment)-1)) & ~((alignment)-1))

/*
 * min, max
 */
#define QKING_MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define QKING_MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))

/**
 * Calculate the index of the first non-zero bit of a 32 bit integer value
 */
#define QKING__LOG2_1(n) (((n) >= 2) ? 1 : 0)
#define QKING__LOG2_2(n) \
  (((n) >= 1 << 2) ? (2 + QKING__LOG2_1((n) >> 2)) : QKING__LOG2_1(n))
#define QKING__LOG2_4(n) \
  (((n) >= 1 << 4) ? (4 + QKING__LOG2_2((n) >> 4)) : QKING__LOG2_2(n))
#define QKING__LOG2_8(n) \
  (((n) >= 1 << 8) ? (8 + QKING__LOG2_4((n) >> 8)) : QKING__LOG2_4(n))
#define QKING_LOG2(n) \
  (((n) >= 1 << 16) ? (16 + QKING__LOG2_8((n) >> 16)) : QKING__LOG2_8(n))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !JRT_H */
