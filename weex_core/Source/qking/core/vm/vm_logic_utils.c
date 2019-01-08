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
#include "ecma_alloc.h"
#include "ecma_builtins.h"
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_function_object.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_lex_env.h"
#include "ecma_objects.h"
#include "ecma_try_catch_macro.h"
#include "vm_defines.h"
#include "vm_logic_utils.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Logical NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_logical_not (ecma_value_t left_value) /**< left value */
{
  return ecma_make_boolean_value (!ecma_op_to_boolean (left_value));
} /* opfunc_logical_not */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_typeof (ecma_value_t left_value) /**< left value */
{
  return ecma_make_magic_string_value (ecma_get_typeof_lit_id (left_value));
} /* opfunc_typeof */

/**
 * Update getter or setter for object literals.
 */
void
opfunc_set_accessor (bool is_getter, /**< is getter accessor */
                     ecma_value_t object, /**< object value */
                     ecma_string_t *accessor_name_p, /**< accessor name */
                     ecma_value_t accessor) /**< accessor value */
{
  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_property_t *property_p = ecma_find_named_property (object_p, accessor_name_p);

  if (property_p != NULL
      && ECMA_PROPERTY_GET_TYPE (*property_p) != ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
  {
    ecma_delete_property (object_p, ECMA_PROPERTY_VALUE_PTR (property_p));
    property_p = NULL;
  }

  if (property_p == NULL)
  {
    ecma_object_t *getter_func_p = NULL;
    ecma_object_t *setter_func_p = NULL;

    if (is_getter)
    {
      getter_func_p = ecma_get_object_from_value (accessor);
    }
    else
    {
      setter_func_p = ecma_get_object_from_value (accessor);
    }

    ecma_create_named_accessor_property (object_p,
                                         accessor_name_p,
                                         getter_func_p,
                                         setter_func_p,
                                         ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE,
                                         NULL);
  }
  else if (is_getter)
  {
    ecma_object_t *getter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_getter (object_p,
                                             ECMA_PROPERTY_VALUE_PTR (property_p),
                                             getter_func_p);
  }
  else
  {
    ecma_object_t *setter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_setter (object_p,
                                             ECMA_PROPERTY_VALUE_PTR (property_p),
                                             setter_func_p);
  }
} /* opfunc_set_accessor */

/**
 * Deletes an object property.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_prop (ecma_value_t object, /**< base object */
                   ecma_value_t property, /**< property name */
                   bool is_strict) /**< strict mode */
{
  if (ecma_is_value_undefined (object))
  {
    return ECMA_VALUE_TRUE;
  }

  ecma_value_t check_coercible = ecma_op_check_object_coercible (object);
  if (ECMA_IS_VALUE_ERROR (check_coercible))
  {
    return check_coercible;
  }
  QKING_ASSERT (check_coercible == ECMA_VALUE_EMPTY);

  ecma_value_t str_name_value = ecma_op_to_string (property);
  if (ECMA_IS_VALUE_ERROR (str_name_value))
  {
    return str_name_value;
  }

  QKING_ASSERT (ecma_is_value_string (str_name_value));
  ecma_string_t *name_string_p = ecma_get_string_from_value (str_name_value);

  ecma_value_t obj_value = ecma_op_to_object (object);
  /* The ecma_op_check_object_coercible call already checked the op_to_object error cases. */
  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (obj_value));
  QKING_ASSERT (ecma_is_value_object (obj_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
  QKING_ASSERT (!ecma_is_lexical_environment (obj_p));

  ecma_value_t delete_op_ret = ecma_op_object_delete (obj_p, name_string_p, is_strict);
  QKING_ASSERT (ecma_is_value_boolean (delete_op_ret) || (is_strict == true && ECMA_IS_VALUE_ERROR (delete_op_ret)));
  ecma_free_value (obj_value);
  ecma_free_value (str_name_value);

  return delete_op_ret;
} /* vm_op_delete_prop */

/**
 * Deletes a variable.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_var (ecma_value_t name_literal, /**< name literal */
                  ecma_object_t *lex_env_p) /**< lexical environment */
{
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;

  ecma_string_t *var_name_str_p = ecma_get_string_from_value (name_literal);

  ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, var_name_str_p);

  if (ref_base_lex_env_p == NULL)
  {
    completion_value = ECMA_VALUE_TRUE;
  }
  else
  {
    QKING_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

    completion_value = ecma_op_delete_binding (ref_base_lex_env_p, var_name_str_p);
  }

  return completion_value;
} /* vm_op_delete_var */

/**
 * 'for-in' opcode handler
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * @return chain list of property names
 */
ecma_collection_chunk_t *
opfunc_for_in (ecma_value_t left_value, /**< left value */
               ecma_value_t *result_obj_p) /**< expression object */
{
  ecma_collection_chunk_t *prop_names_p = NULL;

  /* 3. */
  if (ecma_is_value_undefined (left_value)
      || ecma_is_value_null (left_value))
  {
    return prop_names_p;
  }

  /* 4. */
  ecma_value_t obj_expr_value = ecma_op_to_object (left_value);
  /* ecma_op_to_object will only raise error on null/undefined values but those are handled above. */
  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (obj_expr_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);
  ecma_collection_header_t *prop_names_coll_p = ecma_op_object_get_property_names (obj_p, false, true, true);

  if (prop_names_coll_p->item_count != 0)
  {
    prop_names_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                     prop_names_coll_p->first_chunk_cp);

    ecma_ref_object (obj_p);
    *result_obj_p = ecma_make_object_value (obj_p);
  }

  jmem_heap_free_block (prop_names_coll_p, sizeof (ecma_collection_header_t));
  ecma_free_value (obj_expr_value);

  return prop_names_p;
} /* opfunc_for_in */

/**
 * @}
 * @}
 */
