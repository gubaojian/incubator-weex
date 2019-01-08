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
#include "jcontext.h"

/** \addtogroup context Context
 * @{
 */

#ifndef QKING_ENABLE_EXTERNAL_CONTEXT

/**
 * Global context.
 */
qking_context_t qking_global_context;

#ifndef QKING_SYSTEM_ALLOCATOR

/**
 * Check size of heap is corresponding to configuration
 */
QKING_STATIC_ASSERT (sizeof (jmem_heap_t) <= JMEM_HEAP_SIZE,
                     size_of_mem_heap_must_be_less_than_or_equal_to_JMEM_HEAP_SIZE);

/**
 * Qking global heap section attribute.
 */
#ifndef QKING_HEAP_SECTION_ATTR
#define QKING_GLOBAL_HEAP_SECTION
#else /* QKING_HEAP_SECTION_ATTR */
#define QKING_GLOBAL_HEAP_SECTION QKING_ATTR_SECTION (QKING_HEAP_SECTION_ATTR)
#endif /* !QKING_HEAP_SECTION_ATTR */

/**
 * Global heap.
 */
jmem_heap_t qking_global_heap QKING_ATTR_ALIGNED (JMEM_ALIGNMENT) QKING_GLOBAL_HEAP_SECTION;

#endif /* !QKING_SYSTEM_ALLOCATOR */

#endif /* !QKING_ENABLE_EXTERNAL_CONTEXT */

/**
 * @}
 */
