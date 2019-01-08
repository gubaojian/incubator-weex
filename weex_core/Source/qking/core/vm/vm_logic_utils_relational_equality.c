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
#include "ecma_comparison.h"
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_helpers.h"
#include "ecma_objects.h"
#include "ecma_try_catch_macro.h"
#include "vm_logic_utils.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
* Equality opcode handler.
*
* See also: ECMA-262 v5, 11.9.1, 11.9.2
*
* @return ecma value
*         Returned value must be freed with ecma_free_value
*/
ecma_value_t
opfunc_equality (ecma_value_t left_value, /**< left value */
                 ecma_value_t right_value) /**< right value */
{
  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t compare_result = ecma_op_abstract_equality_compare (left_value, right_value);

  QKING_ASSERT (ecma_is_value_boolean (compare_result)
                || ECMA_IS_VALUE_ERROR (compare_result));

  return compare_result;
} /* opfunc_equality */

/**
 * Relation opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.1, 11.8.2, 11.8.3, 11.8.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_relation (ecma_value_t left_value, /**< left value */
                 ecma_value_t right_value, /**< right value */
                 bool left_first, /**< 'LeftFirst' flag */
                 bool is_invert) /**< is invert */
{
  QKING_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t ret_value = ecma_op_abstract_relational_compare (left_value, right_value, left_first);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = ECMA_VALUE_FALSE;
  }
  else
  {
    QKING_ASSERT (ecma_is_value_boolean (ret_value));

    if (is_invert)
    {
      ret_value = ecma_invert_boolean_value (ret_value);
    }
  }

  return ret_value;
} /* opfunc_relation */

/**
 * 'instanceof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.6
 *
 * @return ecma value
 *         returned value must be freed with ecma_free_value.
 */
ecma_value_t
opfunc_instanceof (ecma_value_t left_value, /**< left value */
                   ecma_value_t right_value) /**< right value */
{
  if (!ecma_is_value_object (right_value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Expected an object in 'instanceof' check."));
  }

  ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);
  return ecma_op_object_has_instance (right_value_obj_p, left_value);
} /* opfunc_instanceof */

/**
 * 'in' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.7
 *
 * @return ecma value
 *         returned value must be freed with ecma_free_value.
 */
ecma_value_t
opfunc_in (ecma_value_t left_value, /**< left value */
           ecma_value_t right_value) /**< right value */
{
  if (!ecma_is_value_object (right_value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Expected an object in 'in' check."));
  }

  bool to_string = !ecma_is_value_string (left_value);

  if (to_string)
  {
    left_value = ecma_op_to_string (left_value);

    if (ECMA_IS_VALUE_ERROR (left_value))
    {
      return left_value;
    }
  }

  ecma_string_t *left_value_prop_name_p = ecma_get_string_from_value (left_value);
  ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);

  ecma_value_t result = ecma_make_boolean_value (ecma_op_object_has_property (right_value_obj_p,
                                                                              left_value_prop_name_p));

  if (to_string)
  {
    ecma_free_value (left_value);
  }
  return result;
} /* opfunc_in */

/**
 * @}
 * @}
 */
