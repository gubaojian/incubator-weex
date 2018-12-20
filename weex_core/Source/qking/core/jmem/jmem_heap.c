/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Heap implementation
 */

#include "base/common_logger.h"
#include "jcontext.h"
#include "jmem.h"
#include "jrt_bit_fields.h"
#include "jrt_libc_includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem_allocator_internal.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

#ifndef QKING_SYSTEM_ALLOCATOR
/**
 * End of list marker.
 */
#define JMEM_HEAP_END_OF_LIST ((uint32_t) 0xffffffff)

/**
 * @{
 */
#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY
/* In this case we simply store the pointer, since it fits anyway. */
#define JMEM_HEAP_GET_OFFSET_FROM_ADDR(p) ((uint32_t) (p))
#define JMEM_HEAP_GET_ADDR_FROM_OFFSET(u) ((jmem_heap_free_t *) (u))
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
#define JMEM_HEAP_GET_OFFSET_FROM_ADDR(p) ((uint32_t) ((uint8_t *) (p) - QKING_HEAP_CONTEXT (area)))
#define JMEM_HEAP_GET_ADDR_FROM_OFFSET(u) ((jmem_heap_free_t *) (QKING_HEAP_CONTEXT (area) + (u)))
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
/**
 * @}
 */

/**
 * Get end of region
 *
 * @return pointer to the end of the region
 */
static inline jmem_heap_free_t *  QKING_ATTR_ALWAYS_INLINE QKING_ATTR_PURE
jmem_heap_get_region_end (jmem_heap_free_t *curr_p) /**< current region */
{
  return (jmem_heap_free_t *)((uint8_t *) curr_p + curr_p->size);
} /* jmem_heap_get_region_end */
#endif /* !QKING_SYSTEM_ALLOCATOR */

/**
 * @{
 * JMEM_HEAP_STAT_xxx definitions
 */
#ifdef JMEM_STATS
static void jmem_heap_stat_init (void);
static void jmem_heap_stat_alloc (size_t num);
static void jmem_heap_stat_free (size_t num);

#define JMEM_HEAP_STAT_INIT() jmem_heap_stat_init ()
#define JMEM_HEAP_STAT_ALLOC(v1) jmem_heap_stat_alloc (v1)
#define JMEM_HEAP_STAT_FREE(v1) jmem_heap_stat_free (v1)

#ifndef QKING_SYSTEM_ALLOCATOR
static void jmem_heap_stat_skip (void);
static void jmem_heap_stat_nonskip (void);
static void jmem_heap_stat_alloc_iter (void);
static void jmem_heap_stat_free_iter (void);

#define JMEM_HEAP_STAT_SKIP() jmem_heap_stat_skip ()
#define JMEM_HEAP_STAT_NONSKIP() jmem_heap_stat_nonskip ()
#define JMEM_HEAP_STAT_ALLOC_ITER() jmem_heap_stat_alloc_iter ()
#define JMEM_HEAP_STAT_FREE_ITER() jmem_heap_stat_free_iter ()
#endif /* !QKING_SYSTEM_ALLOCATOR */
#else /* !JMEM_STATS */
#define JMEM_HEAP_STAT_INIT()
#define JMEM_HEAP_STAT_ALLOC(v1) QKING_UNUSED (v1)
#define JMEM_HEAP_STAT_FREE(v1) QKING_UNUSED (v1)

#ifndef QKING_SYSTEM_ALLOCATOR
#define JMEM_HEAP_STAT_SKIP()
#define JMEM_HEAP_STAT_NONSKIP()
#define JMEM_HEAP_STAT_ALLOC_ITER()
#define JMEM_HEAP_STAT_FREE_ITER()
#endif /* !QKING_SYSTEM_ALLOCATOR */
#endif /* JMEM_STATS */
/** @} */

/**
 * Startup initialization of heap
 */
void
jmem_heap_init (void)
{
#ifndef QKING_SYSTEM_ALLOCATOR
  QKING_ASSERT ((uintptr_t) QKING_HEAP_CONTEXT (area) % JMEM_ALIGNMENT == 0);

  QKING_CONTEXT (jmem_heap_limit) = CONFIG_MEM_HEAP_DESIRED_LIMIT;

  jmem_heap_free_t *const region_p = (jmem_heap_free_t *) QKING_HEAP_CONTEXT (area);

  region_p->size = JMEM_HEAP_AREA_SIZE;
  region_p->next_offset = JMEM_HEAP_END_OF_LIST;

  QKING_HEAP_CONTEXT (first).size = 0;
  QKING_HEAP_CONTEXT (first).next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (region_p);

  QKING_CONTEXT (jmem_heap_list_skip_p) = &QKING_HEAP_CONTEXT (first);

  JMEM_VALGRIND_NOACCESS_SPACE (QKING_HEAP_CONTEXT (area), JMEM_HEAP_AREA_SIZE);

#endif /* !QKING_SYSTEM_ALLOCATOR */
  JMEM_HEAP_STAT_INIT ();
} /* jmem_heap_init */

/**
 * Finalize heap
 */
void
jmem_heap_finalize (void)
{
#ifdef JMEM_STATS
  LOGI("QKING remain heap allocate: %zu",QKING_CONTEXT (jmem_heap_allocated_size));
  LOGI("QKING remain malloc not free: %lu",QKING_CONTEXT (jmem_heap_stats).c_alloc_free_remain);
  QKING_ASSERT(QKING_CONTEXT (jmem_heap_stats).c_alloc_free_remain==0);
#endif
//  QKING_ASSERT (QKING_CONTEXT (jmem_heap_allocated_size) == 0);
#ifndef QKING_SYSTEM_ALLOCATOR
  JMEM_VALGRIND_NOACCESS_SPACE (&QKING_HEAP_CONTEXT (first), JMEM_HEAP_SIZE);
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_heap_finalize */

/**
 * Allocation of memory region.
 *
 * See also:
 *          jmem_heap_alloc_block
 *
 * @return pointer to allocated memory block - if allocation is successful,
 *         NULL - if there is not enough memory.
 */
static void * QKING_ATTR_HOT
jmem_heap_alloc_block_internal (const size_t size) /**< size of requested block */
{
#ifndef QKING_SYSTEM_ALLOCATOR
  /* Align size. */
  const size_t required_size = ((size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT) * JMEM_ALIGNMENT;
  jmem_heap_free_t *data_space_p = NULL;

  JMEM_VALGRIND_DEFINED_SPACE (&QKING_HEAP_CONTEXT (first), sizeof (jmem_heap_free_t));

  /* Fast path for 8 byte chunks, first region is guaranteed to be sufficient. */
  if (required_size == JMEM_ALIGNMENT
      && QKING_LIKELY (QKING_HEAP_CONTEXT (first).next_offset != JMEM_HEAP_END_OF_LIST))
  {
    data_space_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (QKING_HEAP_CONTEXT (first).next_offset);
    QKING_ASSERT (jmem_is_heap_pointer (data_space_p));

    JMEM_VALGRIND_DEFINED_SPACE (data_space_p, sizeof (jmem_heap_free_t));
    QKING_CONTEXT (jmem_heap_allocated_size) += JMEM_ALIGNMENT;
    JMEM_HEAP_STAT_ALLOC_ITER ();

    if (data_space_p->size == JMEM_ALIGNMENT)
    {
      QKING_HEAP_CONTEXT (first).next_offset = data_space_p->next_offset;
    }
    else
    {
      QKING_ASSERT (data_space_p->size > JMEM_ALIGNMENT);

      jmem_heap_free_t *remaining_p;
      remaining_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (QKING_HEAP_CONTEXT (first).next_offset) + 1;

      JMEM_VALGRIND_DEFINED_SPACE (remaining_p, sizeof (jmem_heap_free_t));
      remaining_p->size = data_space_p->size - JMEM_ALIGNMENT;
      remaining_p->next_offset = data_space_p->next_offset;
      JMEM_VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (jmem_heap_free_t));

      QKING_HEAP_CONTEXT (first).next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (remaining_p);
    }

    JMEM_VALGRIND_UNDEFINED_SPACE (data_space_p, sizeof (jmem_heap_free_t));

    if (QKING_UNLIKELY (data_space_p == QKING_CONTEXT (jmem_heap_list_skip_p)))
    {
      QKING_CONTEXT (jmem_heap_list_skip_p) = JMEM_HEAP_GET_ADDR_FROM_OFFSET (QKING_HEAP_CONTEXT (first).next_offset);
    }
  }
  /* Slow path for larger regions. */
  else
  {
    uint32_t current_offset = QKING_HEAP_CONTEXT (first).next_offset;
    jmem_heap_free_t *prev_p = &QKING_HEAP_CONTEXT (first);

    while (current_offset != JMEM_HEAP_END_OF_LIST)
    {
      jmem_heap_free_t *current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (current_offset);
      QKING_ASSERT (jmem_is_heap_pointer (current_p));
      JMEM_VALGRIND_DEFINED_SPACE (current_p, sizeof (jmem_heap_free_t));
      JMEM_HEAP_STAT_ALLOC_ITER ();

      const uint32_t next_offset = current_p->next_offset;
      QKING_ASSERT (next_offset == JMEM_HEAP_END_OF_LIST
                    || jmem_is_heap_pointer (JMEM_HEAP_GET_ADDR_FROM_OFFSET (next_offset)));

      if (current_p->size >= required_size)
      {
        /* Region is sufficiently big, store address. */
        data_space_p = current_p;
        QKING_CONTEXT (jmem_heap_allocated_size) += required_size;


        /* Region was larger than necessary. */
        if (current_p->size > required_size)
        {
          /* Get address of remaining space. */
          jmem_heap_free_t *const remaining_p = (jmem_heap_free_t *) ((uint8_t *) current_p + required_size);

          /* Update metadata. */
          JMEM_VALGRIND_DEFINED_SPACE (remaining_p, sizeof (jmem_heap_free_t));
          remaining_p->size = current_p->size - (uint32_t) required_size;
          remaining_p->next_offset = next_offset;
          JMEM_VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (jmem_heap_free_t));

          /* Update list. */
          JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
          prev_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (remaining_p);
          JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        }
        /* Block is an exact fit. */
        else
        {
          /* Remove the region from the list. */
          JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
          prev_p->next_offset = next_offset;
          JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        }

        QKING_CONTEXT (jmem_heap_list_skip_p) = prev_p;

        /* Found enough space. */
        break;
      }

      JMEM_VALGRIND_NOACCESS_SPACE (current_p, sizeof (jmem_heap_free_t));
      /* Next in list. */
      prev_p = current_p;
      current_offset = next_offset;
    }
  }

  while (QKING_CONTEXT (jmem_heap_allocated_size) >= QKING_CONTEXT (jmem_heap_limit))
  {
    QKING_CONTEXT (jmem_heap_limit) += CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  JMEM_VALGRIND_NOACCESS_SPACE (&QKING_HEAP_CONTEXT (first), sizeof (jmem_heap_free_t));

  if (QKING_UNLIKELY (!data_space_p))
  {
    return NULL;
  }

  QKING_ASSERT ((uintptr_t) data_space_p % JMEM_ALIGNMENT == 0);
  JMEM_VALGRIND_UNDEFINED_SPACE (data_space_p, size);
  JMEM_HEAP_STAT_ALLOC (size);

  return (void *) data_space_p;
#else /* QKING_SYSTEM_ALLOCATOR */
  JMEM_HEAP_STAT_ALLOC (size);
  return jmem_system_malloc (size);
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_heap_alloc_block_internal */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if there is not enough memory.
 *
 * Note:
 *      if there is still not enough memory after running the callbacks
 *        - NULL value will be returned if parmeter 'ret_null_on_error' is true
 *        - the engine will terminate with ERR_OUT_OF_MEMORY if 'ret_null_on_error' is false
 *
 * @return NULL, if the required memory size is 0
 *         also NULL, if 'ret_null_on_error' is true and the allocation fails because of there is not enough memory
 */
static void *
jmem_heap_gc_and_alloc_block (const size_t size,      /**< required memory size */
                              bool ret_null_on_error) /**< indicates whether return null or terminate
                                                           with ERR_OUT_OF_MEMORY on out of memory */
{
  if (QKING_UNLIKELY (size == 0))
  {
    return NULL;
  }

  if (QKING_CONTEXT (jmem_heap_allocated_size) + size >= QKING_CONTEXT (jmem_heap_limit))
  {
    //jmem_run_free_unused_memory_callbacks (JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
  }

  void *data_space_p = jmem_heap_alloc_block_internal (size);

  if (QKING_LIKELY (data_space_p != NULL))
  {
    JMEM_VALGRIND_MALLOCLIKE_SPACE (data_space_p, size);
    return data_space_p;
  }

  for (jmem_free_unused_memory_severity_t severity = JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW;
       severity <= JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH;
       severity = (jmem_free_unused_memory_severity_t) (severity + 1))
  {
    jmem_run_free_unused_memory_callbacks (severity);

    data_space_p = jmem_heap_alloc_block_internal (size);

    if (QKING_LIKELY (data_space_p != NULL))
    {
      JMEM_VALGRIND_MALLOCLIKE_SPACE (data_space_p, size);
      return data_space_p;
    }
  }

  QKING_ASSERT (data_space_p == NULL);

  if (!ret_null_on_error)
  {
    qking_fatal (ERR_OUT_OF_MEMORY);
  }

  return data_space_p;
} /* jmem_heap_gc_and_alloc_block */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if there is not enough memory.
 *
 * Note:
 *      If there is still not enough memory after running the callbacks, then the engine will be
 *      terminated with ERR_OUT_OF_MEMORY.
 *
 * @return NULL, if the required memory is 0
 *         pointer to allocated memory block, otherwise
 */
inline void * QKING_ATTR_HOT QKING_ATTR_ALWAYS_INLINE
jmem_heap_alloc_block (const size_t size)  /**< required memory size */
{
  return jmem_heap_gc_and_alloc_block (size, false);
} /* jmem_heap_alloc_block */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if there is not enough memory.
 *
 * Note:
 *      If there is still not enough memory after running the callbacks, NULL will be returned.
 *
 * @return NULL, if the required memory size is 0
 *         also NULL, if the allocation has failed
 *         pointer to the allocated memory block, otherwise
 */
inline void * QKING_ATTR_HOT QKING_ATTR_ALWAYS_INLINE
jmem_heap_alloc_block_null_on_error (const size_t size) /**< required memory size */
{
  return jmem_heap_gc_and_alloc_block (size, true);
} /* jmem_heap_alloc_block_null_on_error */

#ifdef QKING_ENABLE_GC_DEBUG

void jmem_heap_check() {
    jmem_heap_free_t *prev_p;
    jmem_heap_free_t *next_p;
    prev_p = &QKING_HEAP_CONTEXT (first);
    /* Find position of region in the list. */
    while (prev_p->next_offset < JMEM_HEAP_END_OF_LIST)
    {
        next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (prev_p->next_offset);
        printf("[gc][info][heap]=>[offset][%d][size][%d]\n", prev_p->next_offset, prev_p->size);
        QKING_ASSERT (jmem_is_heap_pointer (next_p));
        
        JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));
        JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        prev_p = next_p;
        
        JMEM_HEAP_STAT_FREE_ITER ();
    }
}

#endif


/**
 * Free the memory block.
 */
void QKING_ATTR_HOT
jmem_heap_free_block (void *ptr, /**< pointer to beginning of data space of the block */
                      const size_t size) /**< size of allocated region */
{
#ifndef QKING_SYSTEM_ALLOCATOR
  /* checking that ptr points to the heap */
  QKING_ASSERT (jmem_is_heap_pointer (ptr));
  QKING_ASSERT (size > 0);
  QKING_ASSERT (QKING_CONTEXT (jmem_heap_limit) >= QKING_CONTEXT (jmem_heap_allocated_size));

  JMEM_VALGRIND_FREELIKE_SPACE (ptr);
  JMEM_VALGRIND_NOACCESS_SPACE (ptr, size);
  JMEM_HEAP_STAT_FREE_ITER ();

  jmem_heap_free_t *block_p = (jmem_heap_free_t *) ptr;
  jmem_heap_free_t *prev_p;
  jmem_heap_free_t *next_p;

  JMEM_VALGRIND_DEFINED_SPACE (&QKING_HEAP_CONTEXT (first), sizeof (jmem_heap_free_t));

  if (block_p > QKING_CONTEXT (jmem_heap_list_skip_p))
  {
    prev_p = QKING_CONTEXT (jmem_heap_list_skip_p);
    JMEM_HEAP_STAT_SKIP ();
  }
  else
  {
    prev_p = &QKING_HEAP_CONTEXT (first);
    JMEM_HEAP_STAT_NONSKIP ();
  }

  QKING_ASSERT (jmem_is_heap_pointer (block_p));
  const uint32_t block_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (block_p);

  JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
  /* Find position of region in the list. */
  while (prev_p->next_offset < block_offset)
  {
    next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (prev_p->next_offset);
    QKING_ASSERT (jmem_is_heap_pointer (next_p));

    JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));
    JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
    prev_p = next_p;

    JMEM_HEAP_STAT_FREE_ITER ();
  }

  next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (prev_p->next_offset);
  JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));

  /* Realign size */
  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;

  JMEM_VALGRIND_DEFINED_SPACE (block_p, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
  /* Update prev. */
  if (jmem_heap_get_region_end (prev_p) == block_p)
  {
    /* Can be merged. */
    prev_p->size += (uint32_t) aligned_size;
    JMEM_VALGRIND_NOACCESS_SPACE (block_p, sizeof (jmem_heap_free_t));
    block_p = prev_p;
  }
  else
  {
    block_p->size = (uint32_t) aligned_size;
    prev_p->next_offset = block_offset;
  }

  JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));
  /* Update next. */
  if (jmem_heap_get_region_end (block_p) == next_p)
  {
    /* Can be merged. */
    block_p->size += next_p->size;
    block_p->next_offset = next_p->next_offset;
  }
  else
  {
    block_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (next_p);
  }

  QKING_CONTEXT (jmem_heap_list_skip_p) = prev_p;

  JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_NOACCESS_SPACE (block_p, size);
  JMEM_VALGRIND_NOACCESS_SPACE (next_p, sizeof (jmem_heap_free_t));

  QKING_ASSERT (QKING_CONTEXT (jmem_heap_allocated_size) > 0);
  QKING_CONTEXT (jmem_heap_allocated_size) -= aligned_size;

  while (QKING_CONTEXT (jmem_heap_allocated_size) + CONFIG_MEM_HEAP_DESIRED_LIMIT <= QKING_CONTEXT (jmem_heap_limit))
  {
    QKING_CONTEXT (jmem_heap_limit) -= CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  JMEM_VALGRIND_NOACCESS_SPACE (&QKING_HEAP_CONTEXT (first), sizeof (jmem_heap_free_t));
  QKING_ASSERT (QKING_CONTEXT (jmem_heap_limit) >= QKING_CONTEXT (jmem_heap_allocated_size));
  JMEM_HEAP_STAT_FREE (size);
#else /* QKING_SYSTEM_ALLOCATOR */
  JMEM_HEAP_STAT_FREE (size);
  jmem_system_free (ptr);
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_heap_free_block */

#ifndef QKING_NDEBUG
/**
 * Check whether the pointer points to the heap
 *
 * Note:
 *      the routine should be used only for assertion checks
 *
 * @return true - if pointer points to the heap,
 *         false - otherwise
 */
bool
jmem_is_heap_pointer (const void *pointer) /**< pointer */
{
#ifndef QKING_SYSTEM_ALLOCATOR
  return ((uint8_t *) pointer >= QKING_HEAP_CONTEXT (area)
          && (uint8_t *) pointer <= (QKING_HEAP_CONTEXT (area) + JMEM_HEAP_AREA_SIZE));
#else /* QKING_SYSTEM_ALLOCATOR */
  QKING_UNUSED (pointer);
  return true;
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_is_heap_pointer */
#endif /* !QKING_NDEBUG */

#ifdef JMEM_STATS
/**
 * Get heap memory usage statistics
 */
void
jmem_heap_get_stats (jmem_heap_stats_t *out_heap_stats_p) /**< [out] heap stats */
{
  QKING_ASSERT (out_heap_stats_p != NULL);

  *out_heap_stats_p = QKING_CONTEXT (jmem_heap_stats);
} /* jmem_heap_get_stats */

/**
 * Print heap memory usage statistics
 */
void
jmem_heap_stats_print (void)
{
  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  LOGD ("Heap stats:\n");
#ifndef QKING_SYSTEM_ALLOCATOR
  LOGD ("  Heap size = %zu bytes\n",
                   heap_stats->size);
#endif /* !QKING_SYSTEM_ALLOCATOR */
  LOGD ("  Allocated = %zu bytes\n"
                   "  Peak allocated = %zu bytes\n"
                   "  Waste = %zu bytes\n"
                   "  Peak waste = %zu bytes\n"
                   "  Allocated byte code data = %zu bytes\n"
                   "  Peak allocated byte code data = %zu bytes\n"
                   "  Allocated string data = %zu bytes\n"
                   "  Peak allocated string data = %zu bytes\n"
                   "  Allocated object data = %zu bytes\n"
                   "  Peak allocated object data = %zu bytes\n"
                   "  Allocated property data = %zu bytes\n"
                   "  Peak allocated property data = %zu bytes\n",
                   heap_stats->allocated_bytes,
                   heap_stats->peak_allocated_bytes,
                   heap_stats->waste_bytes,
                   heap_stats->peak_waste_bytes,
                   heap_stats->byte_code_bytes,
                   heap_stats->peak_byte_code_bytes,
                   heap_stats->string_bytes,
                   heap_stats->peak_string_bytes,
                   heap_stats->object_bytes,
                   heap_stats->peak_object_bytes,
                   heap_stats->property_bytes,
                   heap_stats->peak_property_bytes);
#ifndef QKING_SYSTEM_ALLOCATOR
//  QKING_DEBUG_MSG ("  Skip-ahead ratio = %zu.%04zu\n"
//                   "  Average alloc iteration = %zu.%04zu\n"
//                   "  Average free iteration = %zu.%04zu\n",
//                   heap_stats->skip_count / heap_stats->nonskip_count,
//                   heap_stats->skip_count % heap_stats->nonskip_count * 10000 / heap_stats->nonskip_count,
//                   heap_stats->alloc_iter_count / heap_stats->alloc_count,
//                   heap_stats->alloc_iter_count % heap_stats->alloc_count * 10000 / heap_stats->alloc_count,
//                   heap_stats->free_iter_count / heap_stats->free_count,
//                   heap_stats->free_iter_count % heap_stats->free_count * 10000 / heap_stats->free_count);
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_heap_stats_print */

/**
 * Initalize heap memory usage statistics account structure
 */
static void
jmem_heap_stat_init (void)
{
#ifndef QKING_SYSTEM_ALLOCATOR
  QKING_CONTEXT (jmem_heap_stats).size = JMEM_HEAP_AREA_SIZE;
#endif /* !QKING_SYSTEM_ALLOCATOR */
} /* jmem_heap_stat_init */

/**
 * Account allocation
 */
static void
jmem_heap_stat_alloc (size_t size) /**< Size of allocated block */
{
  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->allocated_bytes += aligned_size;
  heap_stats->waste_bytes += waste_bytes;
  heap_stats->alloc_count++;

  if (heap_stats->allocated_bytes > heap_stats->peak_allocated_bytes)
  {
    heap_stats->peak_allocated_bytes = heap_stats->allocated_bytes;
  }

  if (heap_stats->waste_bytes > heap_stats->peak_waste_bytes)
  {
    heap_stats->peak_waste_bytes = heap_stats->waste_bytes;
  }
} /* jmem_heap_stat_alloc */

/**
 * Account freeing
 */
static void
jmem_heap_stat_free (size_t size) /**< Size of freed block */
{
  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &QKING_CONTEXT (jmem_heap_stats);

  heap_stats->free_count++;
  heap_stats->allocated_bytes -= aligned_size;
  heap_stats->waste_bytes -= waste_bytes;
} /* jmem_heap_stat_free */

#ifndef QKING_SYSTEM_ALLOCATOR
/**
 * Counts number of skip-aheads during insertion of free block
 */
static void
jmem_heap_stat_skip (void)
{
  QKING_CONTEXT (jmem_heap_stats).skip_count++;
} /* jmem_heap_stat_skip  */

/**
 * Counts number of times we could not skip ahead during free block insertion
 */
static void
jmem_heap_stat_nonskip (void)
{
  QKING_CONTEXT (jmem_heap_stats).nonskip_count++;
} /* jmem_heap_stat_nonskip */

/**
 * Count number of iterations required for allocations
 */
static void
jmem_heap_stat_alloc_iter (void)
{
  QKING_CONTEXT (jmem_heap_stats).alloc_iter_count++;
} /* jmem_heap_stat_alloc_iter */

/**
 * Counts number of iterations required for inserting free blocks
 */
static void
jmem_heap_stat_free_iter (void)
{
  QKING_CONTEXT (jmem_heap_stats).free_iter_count++;
} /* jmem_heap_stat_free_iter */
#endif /* !QKING_SYSTEM_ALLOCATOR */
#endif /* JMEM_STATS */

/**
 * @}
 * @}
 */
