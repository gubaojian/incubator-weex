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
#ifndef QKING_COMPILER_H
#define QKING_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \addtogroup qking-compiler Qking compiler compatibility components
 * @{
 */

#ifdef __GNUC__

/*
 * Compiler-specific macros relevant for GCC.
 */
#define QKING_ATTR_ALIGNED(ALIGNMENT) __attribute__((aligned(ALIGNMENT)))
#define QKING_ATTR_ALWAYS_INLINE __attribute__((always_inline))
#define QKING_ATTR_CONST __attribute__((const))
#define QKING_ATTR_DEPRECATED __attribute__((deprecated))
#define QKING_ATTR_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#define QKING_ATTR_HOT __attribute__((hot))
#define QKING_ATTR_NOINLINE __attribute__((noinline))
#define QKING_ATTR_NORETURN __attribute__((noreturn))
#define QKING_ATTR_PURE __attribute__((pure))
#define QKING_ATTR_SECTION(SECTION) __attribute__((section(SECTION)))
#define QKING_ATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#define QKING_LIKELY(x) __builtin_expect(!!(x), 1)
#define QKING_UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif /* __GNUC__ */

#ifdef _MSC_VER

/*
 * Compiler-specific macros relevant for Microsoft Visual C/C++ Compiler.
 */
#define QKING_ATTR_DEPRECATED __declspec(deprecated)
#define QKING_ATTR_NOINLINE __declspec(noinline)
#define QKING_ATTR_NORETURN __declspec(noreturn)

/*
 * Microsoft Visual C/C++ Compiler doesn't support for VLA, using _alloca
 * instead.
 */
void *__cdecl _alloca(size_t _Size);
#define QKING_VLA(type, name, size) \
  type *name = (type *)(_alloca(sizeof(type) * size))

#endif /* _MSC_VER */

/*
 * Default empty definitions for all compiler-specific macros. Define any of
 * these in a guarded block above (e.g., as for GCC) to fine tune compilation
 * for your own compiler. */

/**
 * Function attribute to align function to given number of bytes.
 */
#ifndef QKING_ATTR_ALIGNED
#define QKING_ATTR_ALIGNED(ALIGNMENT)
#endif /* !QKING_ATTR_ALIGNED */

/**
 * Function attribute to inline function to all call sites.
 */
#ifndef QKING_ATTR_ALWAYS_INLINE
#define QKING_ATTR_ALWAYS_INLINE
#endif /* !QKING_ATTR_ALWAYS_INLINE */

/**
 * Function attribute to declare that function has no effect except the return
 * value and it only depends on parameters.
 */
#ifndef QKING_ATTR_CONST
#define QKING_ATTR_CONST
#endif /* !QKING_ATTR_CONST */

/**
 * Function attribute to trigger warning if deprecated function is called.
 */
#ifndef QKING_ATTR_DEPRECATED
#define QKING_ATTR_DEPRECATED
#endif /* !QKING_ATTR_DEPRECATED */

/**
 * Function attribute to declare that function is variadic and takes a format
 * string and some arguments as parameters.
 */
#ifndef QKING_ATTR_FORMAT
#define QKING_ATTR_FORMAT(...)
#endif /* !QKING_ATTR_FORMAT */

/**
 * Function attribute to predict that function is a hot spot, and therefore
 * should be optimized aggressively.
 */
#ifndef QKING_ATTR_HOT
#define QKING_ATTR_HOT
#endif /* !QKING_ATTR_HOT */

/**
 * Function attribute not to inline function ever.
 */
#ifndef QKING_ATTR_NOINLINE
#define QKING_ATTR_NOINLINE
#endif /* !QKING_ATTR_NOINLINE */

/**
 * Function attribute to declare that function never returns.
 */
#ifndef QKING_ATTR_NORETURN
#define QKING_ATTR_NORETURN
#endif /* !QKING_ATTR_NORETURN */

/**
 * Function attribute to declare that function has no effect except the return
 * value and it only depends on parameters and global variables.
 */
#ifndef QKING_ATTR_PURE
#define QKING_ATTR_PURE
#endif /* !QKING_ATTR_PURE */

/**
 * Function attribute to place function in given section.
 */
#ifndef QKING_ATTR_SECTION
#define QKING_ATTR_SECTION(SECTION)
#endif /* !QKING_ATTR_SECTION */

/**
 * Function attribute to trigger warning if function's caller doesn't use (e.g.,
 * check) the return value.
 */
#ifndef QKING_ATTR_WARN_UNUSED_RESULT
#define QKING_ATTR_WARN_UNUSED_RESULT
#endif /* !QKING_ATTR_WARN_UNUSED_RESULT */

/**
 * Helper to predict that a condition is likely.
 */
#ifndef QKING_LIKELY
#define QKING_LIKELY(x) (x)
#endif /* !QKING_LIKELY */

/**
 * Helper to predict that a condition is unlikely.
 */
#ifndef QKING_UNLIKELY
#define QKING_UNLIKELY(x) (x)
#endif /* !QKING_UNLIKELY */

/**
 * Helper to declare (or mimic) a C99 variable-length array.
 */
#ifndef QKING_VLA
#define QKING_VLA(type, name, size) type name[size]
#endif /* !QKING_VLA */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKING_COMPILER_H */
