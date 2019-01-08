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
 * Memory pool manager implementation
 */

#include "jcontext.h"
#include "jmem.h"
#include "jrt_libc_includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem_allocator_internal.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup poolman Memory pool manager
 * @{
 */

/**
 * Finalize pool manager
 */
void
jmem_pools_finalize (void)
{
  jmem_pools_collect_empty ();

  QKING_ASSERT (QKING_CONTEXT (jmem_free_8_byte_chunk_p) == NULL);
  QKING_ASSERT (QKING_CONTEXT (jmem_free_16_byte_chunk_p) == NULL);
} /* jmem_pools_finalize */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
inline void * QKING_ATTR_HOT QKING_ATTR_ALWAYS_INLINE
jmem_pools_alloc (size_t size) /**< size of the chunk */
{
  if (size <= 8)
  {

    if (QKING_CONTEXT (jmem_free_8_byte_chunk_p) != NULL)
    {
      const jmem_pools_chunk_t *const chunk_p = QKING_CONTEXT (jmem_free_8_byte_chunk_p);

      JMEM_VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

      QKING_CONTEXT (jmem_free_8_byte_chunk_p) = chunk_p->next_p;

      JMEM_VALGRIND_UNDEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

      return (void *) chunk_p;
    }
    else
    {
      return (void *) jmem_heap_alloc_block (8);
    }

  }

  QKING_ASSERT (size <= 16);

  if (QKING_CONTEXT (jmem_free_16_byte_chunk_p) != NULL)
  {
    const jmem_pools_chunk_t *const chunk_p = QKING_CONTEXT (jmem_free_16_byte_chunk_p);

    JMEM_VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    QKING_CONTEXT (jmem_free_16_byte_chunk_p) = chunk_p->next_p;

    JMEM_VALGRIND_UNDEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    return (void *) chunk_p;
  }
  else
  {
    return (void *) jmem_heap_alloc_block (16);
  }
} /* jmem_pools_alloc */

/**
 * Free the chunk
 */
inline void QKING_ATTR_HOT QKING_ATTR_ALWAYS_INLINE
jmem_pools_free (void *chunk_p, /**< pointer to the chunk */
                 size_t size) /**< size of the chunk */
{
  QKING_ASSERT (chunk_p != NULL);

  jmem_pools_chunk_t *const chunk_to_free_p = (jmem_pools_chunk_t *) chunk_p;

  JMEM_VALGRIND_DEFINED_SPACE (chunk_to_free_p, size);

  if (size <= 8)
  {

    chunk_to_free_p->next_p = QKING_CONTEXT (jmem_free_8_byte_chunk_p);
    QKING_CONTEXT (jmem_free_8_byte_chunk_p) = chunk_to_free_p;

  }
  else
  {
    QKING_ASSERT (size <= 16);

    chunk_to_free_p->next_p = QKING_CONTEXT (jmem_free_16_byte_chunk_p);
    QKING_CONTEXT (jmem_free_16_byte_chunk_p) = chunk_to_free_p;
  }

  JMEM_VALGRIND_NOACCESS_SPACE (chunk_to_free_p, size);
} /* jmem_pools_free */

/**
 *  Collect empty pool chunks
 */
void
jmem_pools_collect_empty (void)
{
  jmem_pools_chunk_t *chunk_p = QKING_CONTEXT (jmem_free_8_byte_chunk_p);
  QKING_CONTEXT (jmem_free_8_byte_chunk_p) = NULL;

  while (chunk_p)
  {
    JMEM_VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    JMEM_VALGRIND_NOACCESS_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    jmem_heap_free_block (chunk_p, 8);
    chunk_p = next_p;
  }

  chunk_p = QKING_CONTEXT (jmem_free_16_byte_chunk_p);
  QKING_CONTEXT (jmem_free_16_byte_chunk_p) = NULL;

  while (chunk_p)
  {
    JMEM_VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    JMEM_VALGRIND_NOACCESS_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    jmem_heap_free_block (chunk_p, 16);
    chunk_p = next_p;
  }
} /* jmem_pools_collect_empty */

/**
 * @}
 * @}
 */
