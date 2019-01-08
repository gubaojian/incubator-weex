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
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_property_hashmap.h"
#include "jrt_libc_includes.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmapropertyhashmap Property hashmap
 * @{
 */

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE

/**
 * Compute the total size of the property hashmap.
 */
#define ECMA_PROPERTY_HASHMAP_GET_TOTAL_SIZE(max_property_count) \
  (sizeof (ecma_property_hashmap_t) + (max_property_count * sizeof (jmem_cpointer_t)) + (max_property_count >> 3))

/**
 * Number of items in the stepping table.
 */
#define ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS 8

/**
 * Stepping values for searching items in the hashmap.
 */
static const uint8_t ecma_property_hashmap_steps[ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS] QKING_CONST_DATA =
{
  3, 5, 7, 11, 13, 17, 19, 23
};

/**
 * Get the value of a bit in a bitmap.
 */
#define ECMA_PROPERTY_HASHMAP_GET_BIT(byte_p, index) \
  ((byte_p)[(index) >> 3] & (1 << ((index) & 0x7)))

/**
 * Clear the value of a bit in a bitmap.
 */
#define ECMA_PROPERTY_HASHMAP_CLEAR_BIT(byte_p, index) \
  ((byte_p)[(index) >> 3] = (uint8_t) ((byte_p)[(index) >> 3] & ~(1 << ((index) & 0x7))))

/**
 * Set the value of a bit in a bitmap.
 */
#define ECMA_PROPERTY_HASHMAP_SET_BIT(byte_p, index) \
  ((byte_p)[(index) >> 3] = (uint8_t) ((byte_p)[(index) >> 3] | (1 << ((index) & 0x7))))

#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

/**
 * Create a new property hashmap for the object.
 * The object must not have a property hashmap.
 */
void
ecma_property_hashmap_create (ecma_object_t *object_p) /**< object */
{
#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  if (QKING_CONTEXT (ecma_prop_hashmap_alloc_state) != ECMA_PROP_HASHMAP_ALLOC_ON)
  {
    return;
  }

  ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

  if (prop_iter_p == NULL)
  {
    return;
  }

  QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

  uint32_t named_property_count = 0;

  while (prop_iter_p != NULL)
  {
    QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      ecma_property_types_t type = ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[i]);

      if (type != ECMA_PROPERTY_TYPE_SPECIAL)
      {
        named_property_count++;
      }
    }
    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  if (named_property_count < (ECMA_PROPERTY_HASMAP_MINIMUM_SIZE / 2))
  {
    return;
  }

  /* The max_property_count must be power of 2. */
  uint32_t max_property_count = ECMA_PROPERTY_HASMAP_MINIMUM_SIZE;

  /* At least 1/3 items must be NULL. */
  while (max_property_count < (named_property_count + (named_property_count >> 1)))
  {
    max_property_count <<= 1;
  }

  size_t total_size = ECMA_PROPERTY_HASHMAP_GET_TOTAL_SIZE (max_property_count);

  ecma_property_hashmap_t *hashmap_p = (ecma_property_hashmap_t *) jmem_heap_alloc_block_null_on_error (total_size);

  if (hashmap_p == NULL)
  {
    return;
  }

  memset (hashmap_p, 0, total_size);

  hashmap_p->header.types[0] = ECMA_PROPERTY_TYPE_HASHMAP;
  hashmap_p->header.types[1] = 0;
  hashmap_p->header.next_property_cp = object_p->property_list_or_bound_object_cp;
  hashmap_p->max_property_count = max_property_count;
  hashmap_p->null_count = max_property_count - named_property_count;
  hashmap_p->unused_count = max_property_count - named_property_count;

  jmem_cpointer_t *pair_list_p = (jmem_cpointer_t *) (hashmap_p + 1);
  uint8_t *bits_p = (uint8_t *) (pair_list_p + max_property_count);
  uint32_t mask = max_property_count - 1;

  uint8_t shift_counter = 0;

  while (max_property_count > LIT_STRING_HASH_LIMIT)
  {
    shift_counter++;
    max_property_count >>= 1;
  }

  hashmap_p->header.types[1] = shift_counter;

  prop_iter_p = ecma_get_property_list (object_p);
  ECMA_SET_POINTER (object_p->property_list_or_bound_object_cp, hashmap_p);

  while (prop_iter_p != NULL)
  {
    QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if (!ECMA_PROPERTY_IS_NAMED_PROPERTY (prop_iter_p->types[i]))
      {
        continue;
      }

      ecma_property_pair_t *property_pair_p = (ecma_property_pair_t *) prop_iter_p;

      uint32_t entry_index = ecma_string_get_property_name_hash (prop_iter_p->types[i],
                                                                 property_pair_p->names_cp[i]);
      uint32_t step = ecma_property_hashmap_steps[entry_index & (ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS - 1)];

      if (mask < LIT_STRING_HASH_LIMIT)
      {
        entry_index &= mask;
      }
      else
      {
        entry_index <<= shift_counter;
        QKING_ASSERT (entry_index <= mask);
      }

#ifndef QKING_NDEBUG
      /* Because max_property_count (power of 2) and step (a prime
       * number) are relative primes, all entries of the hasmap are
       * visited exactly once before the start entry index is reached
       * again. Furthermore because at least one NULL is present in
       * the hashmap, the while loop must be terminated before the
       * the starting index is reached again. */
      uint32_t start_entry_index = entry_index;
#endif /* !QKING_NDEBUG */

      while (pair_list_p[entry_index] != ECMA_NULL_POINTER)
      {
        entry_index = (entry_index + step) & mask;

#ifndef QKING_NDEBUG
        QKING_ASSERT (entry_index != start_entry_index);
#endif /* !QKING_NDEBUG */
      }

      ECMA_SET_POINTER (pair_list_p[entry_index], property_pair_p);

      if (i != 0)
      {
        ECMA_PROPERTY_HASHMAP_SET_BIT (bits_p, entry_index);
      }
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }
#else /* CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
  QKING_UNUSED (object_p);
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
} /* ecma_property_hashmap_create */

/**
 * Free the hashmap of the object.
 * The object must have a property hashmap.
 */
void
ecma_property_hashmap_free (ecma_object_t *object_p) /**< object */
{
#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  /* Property hash must be exists and must be the first property. */
  ecma_property_header_t *property_p = ecma_get_property_list (object_p);

  QKING_ASSERT (property_p != NULL && property_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP);

  ecma_property_hashmap_t *hashmap_p = (ecma_property_hashmap_t *) property_p;

  object_p->property_list_or_bound_object_cp = property_p->next_property_cp;

  jmem_heap_free_block (hashmap_p,
                        ECMA_PROPERTY_HASHMAP_GET_TOTAL_SIZE (hashmap_p->max_property_count));
#else /* CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
  QKING_UNUSED (object_p);
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
} /* ecma_property_hashmap_free */

/**
 * Insert named property into the hashmap.
 */
void
ecma_property_hashmap_insert (ecma_object_t *object_p, /**< object */
                              ecma_string_t *name_p, /**< name of the property */
                              ecma_property_pair_t *property_pair_p, /**< property pair */
                              int property_index) /**< property index in the pair (0 or 1) */
{
#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  ecma_property_hashmap_t *hashmap_p = ECMA_GET_NON_NULL_POINTER (ecma_property_hashmap_t,
                                                                  object_p->property_list_or_bound_object_cp);

  QKING_ASSERT (hashmap_p->header.types[0] == ECMA_PROPERTY_TYPE_HASHMAP);

  /* The NULLs are reduced below 1/8 of the hashmap. */
  if (hashmap_p->null_count < (hashmap_p->max_property_count >> 3))
  {
    ecma_property_hashmap_free (object_p);
    ecma_property_hashmap_create (object_p);
    return;
  }

  QKING_ASSERT (property_index < ECMA_PROPERTY_PAIR_ITEM_COUNT);

  uint32_t entry_index = ecma_string_hash (name_p);
  uint32_t step = ecma_property_hashmap_steps[entry_index & (ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS - 1)];
  uint32_t mask = hashmap_p->max_property_count - 1;

  if (mask < LIT_STRING_HASH_LIMIT)
  {
    entry_index &= mask;
  }
  else
  {
    entry_index <<= hashmap_p->header.types[1];
  }

#ifndef QKING_NDEBUG
  /* See the comment for this variable in ecma_property_hashmap_create. */
  uint32_t start_entry_index = entry_index;
#endif /* !QKING_NDEBUG */

  jmem_cpointer_t *pair_list_p = (jmem_cpointer_t *) (hashmap_p + 1);

  while (pair_list_p[entry_index] != ECMA_NULL_POINTER)
  {
    entry_index = (entry_index + step) & mask;

#ifndef QKING_NDEBUG
    QKING_ASSERT (entry_index != start_entry_index);
#endif /* !QKING_NDEBUG */
  }

  ECMA_SET_POINTER (pair_list_p[entry_index], property_pair_p);

  uint8_t *bits_p = (uint8_t *) (pair_list_p + hashmap_p->max_property_count);
  bits_p += (entry_index >> 3);
  mask = (uint32_t) (1 << (entry_index & 0x7));

  if (!(*bits_p & mask))
  {
    /* Deleted entries also has ECMA_NULL_POINTER
     * value, but they are not NULL values. */
    hashmap_p->null_count--;
    QKING_ASSERT (hashmap_p->null_count > 0);
  }

  hashmap_p->unused_count--;
  QKING_ASSERT (hashmap_p->unused_count > 0);

  if (property_index == 0)
  {
    *bits_p = (uint8_t) ((*bits_p) & ~mask);
  }
  else
  {
    *bits_p = (uint8_t) ((*bits_p) | mask);
  }
#else /* CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
  QKING_UNUSED (object_p);
  QKING_UNUSED (name_p);
  QKING_UNUSED (property_pair_p);
  QKING_UNUSED (property_index);
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
} /* ecma_property_hashmap_insert */

/**
 * Delete named property from the hashmap.
 *
 * @return ECMA_PROPERTY_HASHMAP_DELETE_RECREATE_HASHMAP if hashmap should be recreated
 *         ECMA_PROPERTY_HASHMAP_DELETE_HAS_HASHMAP otherwise
 */
ecma_property_hashmap_delete_status
ecma_property_hashmap_delete (ecma_object_t *object_p, /**< object */
                              jmem_cpointer_t name_cp, /**< property name */
                              ecma_property_t *property_p) /**< property */
{
#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  ecma_property_hashmap_t *hashmap_p = ECMA_GET_NON_NULL_POINTER (ecma_property_hashmap_t,
                                                                  object_p->property_list_or_bound_object_cp);

  QKING_ASSERT (hashmap_p->header.types[0] == ECMA_PROPERTY_TYPE_HASHMAP);

  hashmap_p->unused_count++;

  /* The NULLs are above 3/4 of the hashmap. */
  if (hashmap_p->unused_count > ((hashmap_p->max_property_count * 3) >> 2))
  {
    return ECMA_PROPERTY_HASHMAP_DELETE_RECREATE_HASHMAP;
  }

  uint32_t entry_index = ecma_string_get_property_name_hash (*property_p, name_cp);
  uint32_t step = ecma_property_hashmap_steps[entry_index & (ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS - 1)];
  uint32_t mask = hashmap_p->max_property_count - 1;
  jmem_cpointer_t *pair_list_p = (jmem_cpointer_t *) (hashmap_p + 1);
  uint8_t *bits_p = (uint8_t *) (pair_list_p + hashmap_p->max_property_count);

  if (mask < LIT_STRING_HASH_LIMIT)
  {
    entry_index &= mask;
  }
  else
  {
    entry_index <<= hashmap_p->header.types[1];
    QKING_ASSERT (entry_index <= mask);
  }

#ifndef QKING_NDEBUG
  /* See the comment for this variable in ecma_property_hashmap_create. */
  uint32_t start_entry_index = entry_index;
#endif /* !QKING_NDEBUG */

  while (true)
  {
    if (pair_list_p[entry_index] != ECMA_NULL_POINTER)
    {
      size_t offset = 0;

      if (ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index))
      {
        offset = 1;
      }

      ecma_property_pair_t *property_pair_p = ECMA_GET_NON_NULL_POINTER (ecma_property_pair_t,
                                                                         pair_list_p[entry_index]);

      if ((property_pair_p->header.types + offset) == property_p)
      {
        QKING_ASSERT (property_pair_p->names_cp[offset] == name_cp);

        pair_list_p[entry_index] = ECMA_NULL_POINTER;
        ECMA_PROPERTY_HASHMAP_SET_BIT (bits_p, entry_index);
        return ECMA_PROPERTY_HASHMAP_DELETE_HAS_HASHMAP;
      }
    }
    else
    {
      /* Must be a deleted entry. */
      QKING_ASSERT (ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index));
    }

    entry_index = (entry_index + step) & mask;

#ifndef QKING_NDEBUG
    QKING_ASSERT (entry_index != start_entry_index);
#endif /* !QKING_NDEBUG */
  }
#else /* CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
  QKING_UNUSED (object_p);
  QKING_UNUSED (name_cp);
  QKING_UNUSED (property_p);
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */
  return ECMA_PROPERTY_HASHMAP_DELETE_HAS_HASHMAP;
} /* ecma_property_hashmap_delete */

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
/**
 * Find a named property.
 *
 * @return pointer to the property if found or NULL otherwise
 */
ecma_property_t *
ecma_property_hashmap_find (ecma_property_hashmap_t *hashmap_p, /**< hashmap */
                            ecma_string_t *name_p, /**< property name */
                            jmem_cpointer_t *property_real_name_cp) /**< [out] property real name */
{
#ifndef QKING_NDEBUG
  /* A sanity check in debug mode: a named property must be present
   * in both the property hashmap and in the property chain, or missing
   * from both data collection. The following code checks the property
   * chain, and sets the property_found variable. */
  bool property_found = false;
  ecma_property_header_t *prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                                          hashmap_p->header.next_property_cp);

  while (prop_iter_p != NULL && !property_found)
  {
    QKING_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if (ECMA_PROPERTY_IS_NAMED_PROPERTY (prop_iter_p->types[i]))
      {
        if (ecma_string_compare_to_property_name (prop_iter_p->types[i],
                                                  prop_pair_p->names_cp[i],
                                                  name_p))
        {
          property_found = true;
          break;
        }
      }
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }
#endif /* !QKING_NDEBUG */

  uint32_t entry_index = ecma_string_hash (name_p);
  uint32_t step = ecma_property_hashmap_steps[entry_index & (ECMA_PROPERTY_HASHMAP_NUMBER_OF_STEPS - 1)];
  uint32_t mask = hashmap_p->max_property_count - 1;
  jmem_cpointer_t *pair_list_p = (jmem_cpointer_t *) (hashmap_p + 1);
  uint8_t *bits_p = (uint8_t *) (pair_list_p + hashmap_p->max_property_count);

  if (mask < LIT_STRING_HASH_LIMIT)
  {
    entry_index &= mask;
  }
  else
  {
    entry_index <<= hashmap_p->header.types[1];
    QKING_ASSERT (entry_index <= mask);
  }

#ifndef QKING_NDEBUG
  /* See the comment for this variable in ecma_property_hashmap_create. */
  uint32_t start_entry_index = entry_index;
#endif /* !QKING_NDEBUG */

  if (ECMA_IS_DIRECT_STRING (name_p))
  {
    ecma_property_t prop_name_type = (ecma_property_t) ECMA_GET_DIRECT_STRING_TYPE (name_p);
    jmem_cpointer_t property_name_cp = (jmem_cpointer_t) ECMA_GET_DIRECT_STRING_VALUE (name_p);

    QKING_ASSERT (prop_name_type > 0);

    while (true)
    {
      if (pair_list_p[entry_index] != ECMA_NULL_POINTER)
      {
        size_t offset = 0;
        if (ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index))
        {
          offset = 1;
        }

        ecma_property_pair_t *property_pair_p = ECMA_GET_NON_NULL_POINTER (ecma_property_pair_t,
                                                                           pair_list_p[entry_index]);

        ecma_property_t *property_p = property_pair_p->header.types + offset;

        QKING_ASSERT (ECMA_PROPERTY_IS_NAMED_PROPERTY (*property_p));

        if (property_pair_p->names_cp[offset] == property_name_cp
            && ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == prop_name_type)
        {
#ifndef QKING_NDEBUG
          QKING_ASSERT (property_found);
#endif /* !QKING_NDEBUG */

          *property_real_name_cp = property_name_cp;
          return property_p;
        }
      }
      else
      {
        if (!ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index))
        {
#ifndef QKING_NDEBUG
          QKING_ASSERT (!property_found);
#endif /* !QKING_NDEBUG */

          return NULL;
        }
        /* Otherwise it is a deleted entry. */
      }

      entry_index = (entry_index + step) & mask;

#ifndef QKING_NDEBUG
      QKING_ASSERT (entry_index != start_entry_index);
#endif /* !QKING_NDEBUG */
    }
  }

  while (true)
  {
    if (pair_list_p[entry_index] != ECMA_NULL_POINTER)
    {
      size_t offset = 0;
      if (ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index))
      {
        offset = 1;
      }

      ecma_property_pair_t *property_pair_p = ECMA_GET_NON_NULL_POINTER (ecma_property_pair_t,
                                                                         pair_list_p[entry_index]);

      ecma_property_t *property_p = property_pair_p->header.types + offset;

      QKING_ASSERT (ECMA_PROPERTY_IS_NAMED_PROPERTY (*property_p));

      if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_DIRECT_STRING_PTR)
      {
        ecma_string_t *prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, property_pair_p->names_cp[offset]);

        if (ecma_compare_ecma_non_direct_strings (prop_name_p, name_p))
        {
#ifndef QKING_NDEBUG
          QKING_ASSERT (property_found);
#endif /* !QKING_NDEBUG */

          *property_real_name_cp = property_pair_p->names_cp[offset];
          return property_p;
        }
      }
    }
    else
    {
      if (!ECMA_PROPERTY_HASHMAP_GET_BIT (bits_p, entry_index))
      {
#ifndef QKING_NDEBUG
        QKING_ASSERT (!property_found);
#endif /* !QKING_NDEBUG */

        return NULL;
      }
      /* Otherwise it is a deleted entry. */
    }

    entry_index = (entry_index + step) & mask;

#ifndef QKING_NDEBUG
    QKING_ASSERT (entry_index != start_entry_index);
#endif /* !QKING_NDEBUG */
  }
} /* ecma_property_hashmap_find */
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

/**
 * @}
 * @}
 */
