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
#include "ecma_builtins.h"
#include "ecma_exceptions.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_typedarray_object.h"
#include "ecma_try_catch_macro.h"
#include "ecma_function_object.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma_builtins_internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma_builtin_typedarray.inc.h"
#define BUILTIN_UNDERSCORED_ID typedarray
#include "ecma_builtin_internal_routines_template.inc.h"

#include "ecma_builtin_typedarray_helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup typedarray ECMA %TypedArray% object built-in
 * @{
 */

/**
 * The %TypedArray%.from routine
 *
 * See also:
 *         ES2015 22.2.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_from (ecma_value_t this_arg, /**< 'this' argument */
                              const ecma_value_t *arguments_list_p, /**< arguments list */
                              ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (!ecma_is_constructor (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a constructor."));
  }

  ecma_value_t source;
  ecma_value_t map_fn = ECMA_VALUE_UNDEFINED;
  ecma_value_t this_in_fn = ECMA_VALUE_UNDEFINED;

  if (arguments_list_len == 0)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("no source argument"));
  }

  source = arguments_list_p[0];

  if (arguments_list_len > 1)
  {
    map_fn = arguments_list_p[1];

    if (!ecma_op_is_callable (map_fn))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("mapfn argument is not callable"));
    }

    if (arguments_list_len > 2)
    {
      this_in_fn = arguments_list_p[2];
    }
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

  const uint8_t builtin_id = ecma_get_object_builtin_id (obj_p);
  if (!ecma_typedarray_helper_is_typedarray (builtin_id))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a typedarray constructor"));
  }

  ecma_object_t *proto_p = ecma_builtin_get (ecma_typedarray_helper_get_prototype_id (builtin_id));
  const uint8_t element_size_shift = ecma_typedarray_helper_get_shift_size (builtin_id);
  const lit_magic_string_id_t class_id = ecma_typedarray_helper_get_magic_string (builtin_id);

  ecma_deref_object (proto_p);

  return ecma_op_typedarray_from (source,
                                  map_fn,
                                  this_in_fn,
                                  proto_p,
                                  element_size_shift,
                                  class_id);

} /* ecma_builtin_typedarray_from */

/**
 * The %TypedArray%.of routine
 *
 * See also:
 *         ES2015 22.2.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_of (ecma_value_t this_arg, /**< 'this' argument */
                            const ecma_value_t *arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_UNUSED (this_arg);
  QKING_UNUSED (arguments_list_p);
  QKING_UNUSED (arguments_list_len);

  /* TODO: implement 'of' */

  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_typedarray_of */

/**
 * Handle calling [[Call]] of built-in %TypedArray% object
 *
 * ES2015 22.2.1 If %TypedArray% is directly called or
 * called as part of a new expression an exception is thrown
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_typedarray_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                       ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray intrinstic cannot be directly called"));
} /* ecma_builtin_typedarray_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in %TypedArray% object
 *
 * ES2015 22.2.1 If %TypedArray% is directly called or
 * called as part of a new expression an exception is thrown
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_typedarray_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                            ecma_length_t arguments_list_len) /**< number of arguments */
{
  QKING_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray intrinstic cannot be called by a 'new' expression"));
} /* ecma_builtin_typedarray_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
