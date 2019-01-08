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
#include "ecma_builtin_helpers.h"
#include "ecma_builtins.h"
#include "ecma_function_object.h"
#include "ecma_gc.h"
#include "ecma_globals.h"
#include "ecma_helpers.h"
#include "ecma_lex_env.h"
#include "ecma_objects.h"
#include "ecma_objects_arguments.h"
#include "ecma_objects_general.h"
#include "ecma_try_catch_macro.h"
#include "jrt.h"

bool ecma_op_is_argument_object(ecma_value_t value) {
  if (!ecma_is_value_object(value)){
    return false;
  }

  ecma_object_t *object = ecma_get_object_from_value(value);
  if(ecma_get_object_type(object) != ECMA_OBJECT_TYPE_CLASS){
    return false;
  }
  return ((ecma_extended_object_t*) object)->u.class_prop.class_id == LIT_MAGIC_STRING_ARGUMENTS_UL;
} /* ecma_op_is_argument_object */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Arguments object creation operation.
 *
 * See also: ECMA-262 v5, 10.6
 */
void
ecma_op_create_arguments_object (ecma_object_t *func_obj_p, /**< callee function */
                                 ecma_object_t *lex_env_p, /**< lexical environment the Arguments
                                                                object is created for */
                                 const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_number, /**< length of arguments list */
                                 const ecma_compiled_code_t *bytecode_data_p) /**< byte code */
{
  bool is_strict = (bytecode_data_p->status_flags & ECMA_CODE_FLAGS_STRICT_MODE) != 0;
  ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
  ecma_object_t *obj_p = ecma_create_object (prototype_p, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_ARGUMENTS_UL;
  ecma_deref_object (prototype_p);
  ecma_property_value_t *prop_value_p;
  /* 11.a, 11.b */
  for (ecma_length_t index = 0; index < arguments_number; index++)
  {
    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

    prop_value_p = ecma_create_named_data_property (obj_p,
                                                    index_string_p,
                                                    ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                    NULL);

    prop_value_p->value = ecma_copy_value_if_not_object (arguments_list_p[index]);

    ecma_deref_ecma_string (index_string_p);
  }

  /* 7. */
  prop_value_p = ecma_create_named_data_property (obj_p,
                                                  ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                                  ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                  NULL);

  prop_value_p->value = ecma_make_uint32_value (arguments_number);

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  /* 13. */
  if (!is_strict)
  {
    prop_value_p = ecma_create_named_data_property (obj_p,
                                                    ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE),
                                                    ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                    NULL);

    prop_value_p->value = ecma_make_object_value (func_obj_p);
  }
  else
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    /* 14. */
    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;

      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = false;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = false;
    }

    ecma_value_t completion = ecma_op_object_define_own_property (obj_p,
                                                                  ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE),
                                                                  &prop_desc,
                                                                  false);

    QKING_ASSERT (ecma_is_value_true (completion));

    completion = ecma_op_object_define_own_property (obj_p,
                                                     ecma_get_magic_string (LIT_MAGIC_STRING_CALLER),
                                                     &prop_desc,
                                                     false);
    QKING_ASSERT (ecma_is_value_true (completion));

    ecma_deref_object (thrower_p);
  }

  ecma_string_t *arguments_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);

  if (is_strict)
  {
    ecma_op_create_immutable_binding (lex_env_p,
                                      arguments_string_p,
                                      ecma_make_object_value (obj_p));
  }
  else
  {
    ecma_value_t completion = ecma_op_create_mutable_binding (lex_env_p,
                                                              arguments_string_p,
                                                              false);
    QKING_ASSERT (ecma_is_value_empty (completion));

    completion = ecma_op_set_mutable_binding (lex_env_p,
                                              arguments_string_p,
                                              ecma_make_object_value (obj_p),
                                              false);

    QKING_ASSERT (ecma_is_value_empty (completion));
  }

  ecma_deref_object (obj_p);
} /* ecma_op_create_arguments_object */

/**
 * [[DefineOwnProperty]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_arguments_object_define_own_property (ecma_object_t *object_p, /**< the object */
                                              ecma_string_t *property_name_p, /**< property name */
                                              const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                                  *   descriptor */
                                              bool is_throw) /**< flag that controls failure handling */
{
  /* 3. */
  ecma_value_t ret_value = ecma_op_general_object_define_own_property (object_p,
                                                                       property_name_p,
                                                                       property_desc_p,
                                                                       is_throw);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ret_value;
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  if (index >= ext_object_p->u.pseudo_array.u1.length)
  {
    return ret_value;
  }

  ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

  if (arg_Literal_p[index] == ECMA_VALUE_EMPTY)
  {
    return ret_value;
  }

  ecma_string_t *name_p = ecma_get_string_from_value (arg_Literal_p[index]);

  if (property_desc_p->is_get_defined
      || property_desc_p->is_set_defined)
  {
    ecma_deref_ecma_string (name_p);
    arg_Literal_p[index] = ECMA_VALUE_EMPTY;
  }
  else
  {
    if (property_desc_p->is_value_defined)
    {
      /* emulating execution of function described by MakeArgSetter */
      ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                  ext_object_p->u.pseudo_array.u2.lex_env_cp);

      ecma_value_t completion = ecma_op_set_mutable_binding (lex_env_p,
                                                             name_p,
                                                             property_desc_p->value,
                                                             true);

      QKING_ASSERT (ecma_is_value_empty (completion));
    }

    if (property_desc_p->is_writable_defined
        && !property_desc_p->is_writable)
    {
      ecma_deref_ecma_string (name_p);
      arg_Literal_p[index] = ECMA_VALUE_EMPTY;
    }
  }

  return ret_value;
} /* ecma_op_arguments_object_define_own_property */

/**
 * [[Delete]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_arguments_object_delete (ecma_object_t *object_p, /**< the object */
                                 ecma_string_t *property_name_p, /**< property name */
                                 bool is_throw) /**< flag that controls failure handling */
{
  /* 3. */
  ecma_value_t ret_value = ecma_op_general_object_delete (object_p, property_name_p, is_throw);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  QKING_ASSERT (ecma_is_value_boolean (ret_value));

  if (ecma_is_value_true (ret_value))
  {
    uint32_t index = ecma_string_get_array_index (property_name_p);

    if (index != ECMA_STRING_NOT_ARRAY_INDEX)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (index < ext_object_p->u.pseudo_array.u1.length)
      {
        ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

        if (arg_Literal_p[index] != ECMA_VALUE_EMPTY)
        {
          ecma_string_t *name_p = ecma_get_string_from_value (arg_Literal_p[index]);
          ecma_deref_ecma_string (name_p);
          arg_Literal_p[index] = ECMA_VALUE_EMPTY;
        }
      }
    }

    ret_value = ECMA_VALUE_TRUE;
  }

  return ret_value;
} /* ecma_op_arguments_object_delete */

/**
 * @}
 * @}
 */
