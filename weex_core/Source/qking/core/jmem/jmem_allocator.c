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
/**
 * Allocator implementation
 */
#include "ecma_globals.h"
#include "jcontext.h"
#include "jmem.h"
#include "jrt_libc_includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem_allocator_internal.h"

#ifdef JMEM_STATS
/**
 * Register byte code allocation.
 */
void
jmem_stats_allocate_byte_code_bytes (size_t byte_code_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->byte_code_bytes += byte_code_size;

  if (heap_stats->byte_code_bytes >= heap_stats->peak_byte_code_bytes)
  {
    heap_stats->peak_byte_code_bytes = heap_stats->byte_code_bytes;
  }
} /* jmem_stats_allocate_byte_code_bytes */

/**
 * Register byte code free.
 */
void
jmem_stats_free_byte_code_bytes (size_t byte_code_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  QKING_ASSERT (heap_stats->byte_code_bytes >= byte_code_size);

  heap_stats->byte_code_bytes -= byte_code_size;
} /* jmem_stats_free_byte_code_bytes */

/**
 * Register string allocation.
 */
void
jmem_stats_allocate_string_bytes (size_t string_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->string_bytes += string_size;

  if (heap_stats->string_bytes >= heap_stats->peak_string_bytes)
  {
    heap_stats->peak_string_bytes = heap_stats->string_bytes;
  }
} /* jmem_stats_allocate_string_bytes */

/**
 * Register string free.
 */
void
jmem_stats_free_string_bytes (size_t string_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  QKING_ASSERT (heap_stats->string_bytes >= string_size);

  heap_stats->string_bytes -= string_size;
} /* jmem_stats_free_string_bytes */

/**
 * Register object allocation.
 */
void
jmem_stats_allocate_object_bytes (size_t object_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->object_bytes += object_size;

  if (heap_stats->object_bytes >= heap_stats->peak_object_bytes)
  {
    heap_stats->peak_object_bytes = heap_stats->object_bytes;
  }
} /* jmem_stats_allocate_object_bytes */

/**
 * Register object free.
 */
void
jmem_stats_free_object_bytes (size_t object_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  QKING_ASSERT (heap_stats->object_bytes >= object_size);

  heap_stats->object_bytes -= object_size;
} /* jmem_stats_free_object_bytes */

/**
 * Register property allocation.
 */
void
jmem_stats_allocate_property_bytes (size_t property_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->property_bytes += property_size;

  if (heap_stats->property_bytes >= heap_stats->peak_property_bytes)
  {
    heap_stats->peak_property_bytes = heap_stats->property_bytes;
  }
} /* jmem_stats_allocate_property_bytes */

/**
 * Register property free.
 */
void
jmem_stats_free_property_bytes (size_t property_size)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  QKING_ASSERT (heap_stats->property_bytes >= property_size);

  heap_stats->property_bytes -= property_size;
} /* jmem_stats_free_property_bytes */

#endif /* JMEM_STATS */

/**
 * Initialize memory allocators.
 */
void
jmem_init (void)
{
  jmem_heap_init ();
} /* jmem_init */

/**
 * Finalize memory allocators.
 */
void
jmem_finalize (void)
{
  jmem_pools_finalize ();

#ifdef JMEM_STATS
  if (QKING_CONTEXT (qking_init_flags) & ECMA_INIT_MEM_STATS)
  {
    jmem_heap_stats_print ();
  }
#endif /* JMEM_STATS */

  jmem_heap_finalize ();
} /* jmem_finalize */

/**
 * Compress pointer
 *
 * @return packed pointer
 */
inline jmem_cpointer_t QKING_ATTR_PURE QKING_ATTR_ALWAYS_INLINE
jmem_compress_pointer (const void *pointer_p) /**< pointer to compress */
{
  QKING_ASSERT (pointer_p != NULL);
  QKING_ASSERT (jmem_is_heap_pointer (pointer_p));

  uintptr_t uint_ptr = (uintptr_t) pointer_p;

  QKING_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);

#if defined (ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY)
  QKING_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
  const uintptr_t heap_start = (uintptr_t) &QKING_HEAP_CONTEXT (first);

  uint_ptr -= heap_start;
  uint_ptr >>= JMEM_ALIGNMENT_LOG;

  QKING_ASSERT (uint_ptr <= UINT32_MAX);
  QKING_ASSERT (uint_ptr != JMEM_CP_NULL);
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  return (jmem_cpointer_t) uint_ptr;
} /* jmem_compress_pointer */

/**
 * Decompress pointer
 *
 * @return unpacked pointer
 */
inline void * QKING_ATTR_PURE QKING_ATTR_ALWAYS_INLINE
jmem_decompress_pointer (uintptr_t compressed_pointer) /**< pointer to decompress */
{
  QKING_ASSERT (compressed_pointer != JMEM_CP_NULL);

  uintptr_t uint_ptr = compressed_pointer;

  QKING_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);

#if defined (ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY)
  QKING_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
  const uintptr_t heap_start = (uintptr_t) &QKING_HEAP_CONTEXT (first);

  uint_ptr <<= JMEM_ALIGNMENT_LOG;
  uint_ptr += heap_start;

  QKING_ASSERT (jmem_is_heap_pointer ((void *) uint_ptr));
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  return (void *) uint_ptr;
} /* jmem_decompress_pointer */

/**
 * Register specified 'try to give memory back' callback routine
 */
void
jmem_register_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback) /**< callback routine */
{
  /* Currently only one callback is supported */
  QKING_ASSERT (QKING_CONTEXT (jmem_free_unused_memory_callback) == NULL);

  QKING_CONTEXT (jmem_free_unused_memory_callback) = callback;
} /* jmem_register_free_unused_memory_callback */

/**
 * Unregister specified 'try to give memory back' callback routine
 */
void
jmem_unregister_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback) /**< callback routine */
{
  /* Currently only one callback is supported */
  QKING_ASSERT (QKING_CONTEXT (jmem_free_unused_memory_callback) == callback);

  QKING_CONTEXT (jmem_free_unused_memory_callback) = NULL;
} /* jmem_unregister_free_unused_memory_callback */

/**
 * Run 'try to give memory back' callbacks with specified severity
 */
void
jmem_run_free_unused_memory_callbacks (jmem_free_unused_memory_severity_t severity) /**< severity of the request */
{
  if (QKING_CONTEXT (jmem_free_unused_memory_callback) != NULL)
  {
    QKING_CONTEXT (jmem_free_unused_memory_callback) (severity);
  }

  jmem_pools_collect_empty ();
}

inline void* jmem_system_malloc(size_t size) {
#ifdef JMEM_STATS
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);
  heap_stats->c_alloc_free_remain+=size;
  size_t * ret = malloc(sizeof(size_t) + size);
  *ret = size;
  return &ret[1];
#else
  return malloc(size);
#endif
}

inline void jmem_system_free(void *ptr) {
#ifdef JMEM_STATS
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);
  heap_stats->c_alloc_free_remain-= *((size_t*)ptr - 1);
  free( (size_t*)ptr - 1);
#else
  free(ptr);
#endif
}
/* jmem_run_free_unused_memory_callbacks */
