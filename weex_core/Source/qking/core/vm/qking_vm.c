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
#include "core/ecma/operations/ecma_objects_arguments.h"
#include "qking_vm.h"
#include "base/common_logger.h"
#include "ecma_alloc.h"
#include "ecma_array_object.h"
#include "ecma_builtins.h"
#include "ecma_comparison.h"
#include "ecma_conversion.h"
#include "ecma_exceptions.h"
#include "ecma_function_object.h"
#include "ecma_gc.h"
#include "ecma_helpers.h"
#include "ecma_lcache.h"
#include "ecma_lex_env.h"
#include "ecma_map_object.h"
#include "ecma_objects.h"
#include "handler/handler.h"
#include "jcontext.h"
#include "qking_core.h"
#include "vm_logic_utils.h"
#include "vm_op_code.h"
#include "vm_ecma_value_helper.h"

/**
 * Update getter or setter for object literals.
 */
static void vm_op_set_accessor(
    bool is_getter,                 /**< is getter accessor */
    ecma_value_t object,            /**< object value */
    ecma_string_t *accessor_name_p, /**< accessor name */
    ecma_value_t accessor)          /**< accessor value */
{
  ecma_object_t *object_p = ecma_get_object_from_value(object);
  ecma_property_t *property_p =
      ecma_find_named_property(object_p, accessor_name_p);

  if (property_p != NULL &&
      ECMA_PROPERTY_GET_TYPE(*property_p) != ECMA_PROPERTY_TYPE_NAMEDACCESSOR) {
    ecma_delete_property(object_p, ECMA_PROPERTY_VALUE_PTR(property_p));
    property_p = NULL;
  }

  if (property_p == NULL) {
    ecma_object_t *getter_func_p = NULL;
    ecma_object_t *setter_func_p = NULL;

    if (is_getter) {
      getter_func_p = ecma_get_object_from_value(accessor);
    } else {
      setter_func_p = ecma_get_object_from_value(accessor);
    }

    ecma_create_named_accessor_property(
        object_p, accessor_name_p, getter_func_p, setter_func_p,
        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE, NULL);
  } else if (is_getter) {
    ecma_object_t *getter_func_p = ecma_get_object_from_value(accessor);

    ecma_set_named_accessor_property_getter(
        object_p, ECMA_PROPERTY_VALUE_PTR(property_p), getter_func_p);
  } else {
    ecma_object_t *setter_func_p = ecma_get_object_from_value(accessor);

    ecma_set_named_accessor_property_setter(
        object_p, ECMA_PROPERTY_VALUE_PTR(property_p), setter_func_p);
  }
} /* opfunc_set_accessor */

static ecma_value_t vm_op_for_in(ecma_value_t *lhs, ecma_value_t rhs) {
  bool condition = false;
  do {
    if (ecma_is_value_undefined(rhs) || ecma_is_value_null(rhs)) {
      break;
    }
    /* 4. */
    ecma_value_t obj_expr_value = ecma_op_to_object(rhs);
    /* ecma_op_to_object will only raise error on null/undefined values but
     * those are handled above. */
    QKING_ASSERT(!ECMA_IS_VALUE_ERROR(obj_expr_value));
    ecma_object_t *obj_p = ecma_get_object_from_value(obj_expr_value);
    ecma_collection_header_t *prop_names_coll_p =
        ecma_op_object_get_property_names(obj_p, false, true, true);

    ecma_value_t *ecma_value_p = ecma_collection_iterator_init(prop_names_coll_p);
    if (!ecma_value_p) {
        ecma_free_value(obj_expr_value);
        ecma_free_values_collection(prop_names_coll_p, 0);
        break;
    }
    if (ecma_is_value_undefined(*lhs) || ecma_is_value_null(*lhs)) {
        *lhs = ecma_fast_copy_value(*ecma_value_p);
        condition = true;
    }
    else {
        bool finder = false;
        ecma_value_t first_value = *ecma_value_p;
        while (ecma_value_p != NULL) {
            if (*ecma_value_p == *lhs) {
                finder = true;
                break;
            }
            ecma_value_p = ecma_collection_iterator_next(ecma_value_p);
        }
        if (!finder) {
          ecma_fast_free_value(*lhs);
          *lhs = ecma_fast_copy_value(first_value);
          condition = true;
        }
        else {
          ecma_value_p = ecma_collection_iterator_next(ecma_value_p);
          if (ecma_value_p) {
              ecma_fast_free_value(*lhs);
              *lhs = ecma_fast_copy_value(*ecma_value_p);
              condition = true;
          }
          else {
              ecma_fast_free_value(*lhs);
              *lhs = ECMA_VALUE_UNDEFINED;
          }
        }
    }
    ecma_free_value(obj_expr_value);
    ecma_free_values_collection (prop_names_coll_p, 0);

  } while (0);

  return ecma_make_boolean_value(condition);
}

static ecma_object_t *vm_op_new_constructor_object() {
  ecma_object_t *prototype_obj_p =
      ecma_builtin_get(ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);
  ecma_object_t *function_obj_p =
      ecma_create_object(prototype_obj_p, sizeof(ecma_extended_object_t),
                         ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
  ecma_deref_object(prototype_obj_p);
  ecma_extended_object_t *ext_func_obj_p =
      (ecma_extended_object_t *)function_obj_p;
  ext_func_obj_p->u.function.closure_cp = ECMA_NULL_POINTER;
  ext_func_obj_p->u.external_handler_cb =
      ecma_op_function_implicit_constructor_handler_cb;
  return function_obj_p;
}

/**
 * Get the value of object[property].
 *
 * @return ecma value
 */
static ecma_value_t vm_op_get_value(ecma_value_t object,   /**< base object */
                                    ecma_value_t property) /**< property name */
{
  if (ecma_is_value_object(object)) {
    ecma_string_t *property_name_p = NULL;

    if (ecma_is_value_integer_number(property)) {
      ecma_integer_value_t int_value = ecma_get_integer_from_value(property);

      if (int_value >= 0 && int_value <= ECMA_DIRECT_STRING_MAX_IMM) {
        property_name_p = (ecma_string_t *)ECMA_CREATE_DIRECT_STRING(
            ECMA_DIRECT_STRING_UINT, (uintptr_t)int_value);
      }
    } else if (ecma_is_value_string(property)) {
      property_name_p = ecma_get_string_from_value(property);
    }

#ifdef DEBUG_LOG_ENABLE
    QKING_GET_LOG_STR_START(debug_print_str, property_name_p);
    LOGD("-- Get property: %s direct: %s",debug_print_str, ecma_is_value_direct_string(property)?"true":"false");
    QKING_GET_LOG_STR_FINISH(debug_print_str);
#endif

    if (property_name_p != NULL) {
#ifndef CONFIG_ECMA_LCACHE_DISABLE
      ecma_object_t *object_p = ecma_get_object_from_value(object);
      ecma_property_t *property_p =
          ecma_lcache_lookup(object_p, property_name_p);

      if (property_p != NULL &&
          ECMA_PROPERTY_GET_TYPE(*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA) {
        return ecma_fast_copy_value(ECMA_PROPERTY_VALUE_PTR(property_p)->value);
      }
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

      /* There is no need to free the name. */
      return ecma_op_object_get(ecma_get_object_from_value(object),
                                property_name_p);
    }
  }

  if (QKING_UNLIKELY(ecma_is_value_undefined(object) ||
                     ecma_is_value_null(object))) {
#ifdef QKING_ENABLE_ERROR_MESSAGES
    ecma_value_t error_value = ecma_raise_standard_error_with_format(
        ECMA_ERROR_TYPE, "Cannot read property '%' of %", property, object);
#else  /* !QKING_ENABLE_ERROR_MESSAGES */
    ecma_value_t error_value = ecma_raise_type_error(NULL);
#endif /* QKING_ENABLE_ERROR_MESSAGES */
    return error_value;
  }

  ecma_value_t prop_to_string_result = ecma_op_to_string(property);

  if (ECMA_IS_VALUE_ERROR(prop_to_string_result)) {
    return prop_to_string_result;
  }

  ecma_string_t *property_name_p =
      ecma_get_string_from_value(prop_to_string_result);

  ecma_value_t get_value_result =
      ecma_op_get_value_object_base(object, property_name_p);

  ecma_deref_ecma_string(property_name_p);
  return get_value_result;
} /* vm_op_get_value */

static ecma_value_t vm_op_inherit(vm_frame_ctx_t *frame_ctx_p, ecma_value_t child_value,
                          ecma_value_t super_value) {
  ecma_value_t completion_value =  ECMA_VALUE_UNDEFINED;
  do {
    ecma_object_t *super_class_p;
    ecma_value_t result = ECMA_VALUE_UNDEFINED;
    if (ecma_is_value_null(super_value)) {
      super_class_p = ecma_create_object(NULL, 0, ECMA_OBJECT_TYPE_GENERAL);
    } else {
      result = ecma_op_to_object(super_value);
      ecma_free_value(super_value);
      if (ECMA_IS_VALUE_ERROR(result) || !ecma_is_constructor(result)) {
        ecma_free_value(result);
        ecma_fast_free_value(QKING_CONTEXT(error_value));
        completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("super isn't a constructor OP_INHERITANCE"));
        break;
      } else {
        super_class_p = ecma_get_object_from_value(result);
      }
    }
    ecma_object_t *child_class_p = ecma_get_object_from_value(child_value);
    ecma_value_t child_prototype_value = ecma_op_object_get_by_magic_id(
        child_class_p, LIT_MAGIC_STRING_PROTOTYPE);
    ecma_object_t *child_prototype_class_p =
        ecma_get_object_from_value(child_prototype_value);
    ecma_property_value_t *prop_value_p;
    prop_value_p = ecma_create_named_data_property(
        child_prototype_class_p,
        ecma_get_magic_string(LIT_MAGIC_STRING_CONSTRUCTOR),
        ECMA_PROPERTY_CONFIGURABLE_WRITABLE, NULL);
    ecma_named_data_property_assign_value(child_prototype_class_p, prop_value_p,
                                          child_value);
    if (ecma_get_object_prototype(super_class_p)) {
      ecma_value_t super_prototype_value = ecma_op_object_get_by_magic_id(
          super_class_p, LIT_MAGIC_STRING_PROTOTYPE);
      if (ecma_get_object_type(super_class_p) ==
              ECMA_OBJECT_TYPE_BOUND_FUNCTION &&
          !ecma_is_value_object(super_prototype_value)) {
        ecma_free_value(super_prototype_value);

        ecma_fast_free_value(QKING_CONTEXT(error_value));
        completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("class extends value does not have valid prototype property"));
        break;
      }
      if (!(ECMA_IS_VALUE_ERROR(super_prototype_value) ||
            !ecma_is_value_object(super_prototype_value))) {
        ecma_object_t *super_prototype_class_p =
            ecma_get_object_from_value(super_prototype_value);

        ECMA_SET_POINTER(
            child_prototype_class_p->prototype_or_outer_reference_cp,
            super_prototype_class_p);
        ECMA_SET_POINTER(child_class_p->prototype_or_outer_reference_cp,
                         super_class_p);
      }
      ecma_free_value(super_prototype_value);
    }
    ecma_free_value(child_prototype_value);

  } while (0);
  return completion_value;
}

/**
 * Set the value of object[property].
 *
 *
 * @return an ecma value which contains an error
 *         if the property setting is unsuccessful
 */
static ecma_value_t vm_op_set_value(ecma_value_t *object,  /**< base object */
                                    ecma_value_t property, /**< property name */
                                    ecma_value_t value,    /**< ecma value */
                                    bool is_strict)        /**< strict mode */
{
  if (QKING_UNLIKELY(!ecma_is_value_object(*object))) {
    ecma_value_t to_object = ecma_op_to_object(*object);
    if (ECMA_IS_VALUE_ERROR(to_object)) {
#ifdef QKING_ENABLE_ERROR_MESSAGES
      ecma_free_value(to_object);
      ecma_free_value(QKING_CONTEXT(error_value));
      QKING_CONTEXT(error_value) = ECMA_VALUE_UNDEFINED;
      ecma_value_t error_value = ecma_raise_standard_error_with_format(
          ECMA_ERROR_TYPE, "Cannot set property '%' of %", property, *object);
      return error_value;
#else  /* !QKING_ENABLE_ERROR_MESSAGES */
      return to_object;
#endif /* QKING_ENABLE_ERROR_MESSAGES */
    }
    ecma_fast_free_value(*object);
    *object = to_object;
  }
  bool is_property_free = false;
  if (!ecma_is_value_string(property)) {
    ecma_value_t to_string = ecma_op_to_string(property);
    if (ECMA_IS_VALUE_ERROR(to_string)) {
      return to_string;
    }
    property = to_string;
    is_property_free = true;
  }

  ecma_object_t *object_p = ecma_get_object_from_value(*object);
#ifdef DEBUG_LOG_ENABLE
  QKING_TO_LOG_STR_START(debug_print_str, property);
  LOGD("-- Set property: %s direct: %s",debug_print_str, ecma_is_value_direct_string(property)?"true":"false");
  QKING_TO_LOG_STR_FINISH(debug_print_str);
#endif
  ecma_string_t *property_p = ecma_get_string_from_value(property);
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;

  if (!ecma_is_lexical_environment(object_p)) {
    completion_value =
        ecma_op_object_put(object_p, property_p, value, is_strict);
  } else {
    completion_value =
        ecma_op_set_mutable_binding(object_p, property_p, value, is_strict);
  }
  if (is_property_free) {
      ecma_free_value(property);
  }
  return completion_value;
} /* vm_op_set_value */

static ecma_value_t vm_op_set_object_keys_value(ecma_value_t *object,
                                                ecma_value_t prop_obj_var,
                                                bool is_strict) {
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;
  ecma_object_t *prop_obj_p = ecma_get_object_from_value(prop_obj_var);
  ecma_collection_header_t *props_p =
      ecma_op_object_get_property_names(prop_obj_p, false, true, false);
  ecma_value_t *ecma_value_p = ecma_collection_iterator_init(props_p);

  while (ecma_value_p != NULL) {
    ecma_value_t object_prop_var = vm_op_get_value(prop_obj_var, *ecma_value_p);
    if (ECMA_IS_VALUE_ERROR(object_prop_var)) {
      completion_value = object_prop_var;
      break;
    }
    completion_value =
        vm_op_set_value(object, *ecma_value_p, object_prop_var, is_strict);
    if (ECMA_IS_VALUE_ERROR(completion_value)) {
      ecma_fast_free_value(object_prop_var);
      break;
    }
    ecma_fast_free_value(object_prop_var);
    ecma_value_p = ecma_collection_iterator_next(ecma_value_p);
  }

  ecma_free_values_collection(props_p, 0);
  return completion_value;
}
//caller, this, unfold, arg0, arg1, ...
static void ecma_func_call(vm_frame_ctx_t *frame_ctx_p, ecma_register_t *func,
                           const uint32_t argc, ecma_register_t *ret) {
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;
  ecma_object_t *func_obj_p = ecma_get_object_from_value(func->var);
  ecma_value_t this_value = func[1].var;
  ecma_value_t unfold_array = func[2].var;

  QKING_VLA(bool, is_unfolds, argc);
  for (size_t j = 0; j < argc; ++j) {
    is_unfolds[j] = false;
  }
  bool has_unfold = false;
  if (ecma_is_value_object_array(unfold_array)) {
    has_unfold = true;
    //has unfold arg
    ecma_object_t *prop_obj_p = ecma_get_object_from_value(unfold_array);
    ecma_collection_header_t *props_p =
        ecma_op_object_get_property_names(prop_obj_p, true, true, false);
    ecma_value_t *ecma_value_p = ecma_collection_iterator_init(props_p);
    
    while (ecma_value_p != NULL) {
      ecma_value_t object_prop_var = vm_op_get_value(unfold_array, *ecma_value_p);
      QKING_ASSERT(!ECMA_IS_VALUE_ERROR(object_prop_var));
      QKING_ASSERT(ecma_is_value_integer_number(object_prop_var));

      ecma_integer_value_t number = ecma_get_integer_from_value(object_prop_var);
      QKING_ASSERT(number<argc);
      is_unfolds[number] = true;
      
      ecma_fast_free_value(object_prop_var);
      ecma_value_p = ecma_collection_iterator_next(ecma_value_p);
    }

    ecma_free_values_collection(props_p, 0);
  } else {
    QKING_ASSERT(unfold_array == ECMA_VALUE_UNDEFINED);
  }

  ecma_length_t actural_argc = 0;
  if (has_unfold) {
    for (size_t j = 0; j < argc; ++j) {
      if (is_unfolds[j]) {
        ecma_value_t unfold_arg = func[3 + j].var;
        if (!ecma_is_value_object_array(unfold_arg) && !ecma_op_is_argument_object(unfold_arg)){
#ifdef QKING_ENABLE_ERROR_MESSAGES
          completion_value = ecma_raise_standard_error_with_format(
              ECMA_ERROR_TYPE, "Found non-callable @@iterator");
#else  /* !QKING_ENABLE_ERROR_MESSAGES */
          completion_value = ecma_raise_type_error(NULL);
#endif /* QKING_ENABLE_ERROR_MESSAGES */

          ecma_fast_free_value(ret->var);
          ret->var = completion_value;
          return;
        }

        //is array;
        ecma_object_t *prop_obj_p = ecma_get_object_from_value(unfold_arg);
        ecma_collection_header_t *props_p =
            ecma_op_object_get_property_names(prop_obj_p, true, true, false);
        actural_argc+=props_p->item_count;
        ecma_free_values_collection(props_p, 0);
      } else {
        actural_argc+=1;
      }
    }
  } else {
    actural_argc = argc;
  }

  LOGD("FUNC CALL actural_argc: %u",actural_argc);

  QKING_VLA(ecma_value_t, args, actural_argc);
  size_t actural_index = 0;
  for (int i = 0; i < argc; i++) {
    ecma_value_t current_arg = func[3 + i].var;
    if (is_unfolds[i]) {
      QKING_ASSERT(ecma_is_value_object_array(current_arg) || ecma_op_is_argument_object(current_arg));
      //is array;
      ecma_object_t *prop_obj_p = ecma_get_object_from_value(current_arg);
      ecma_collection_header_t *props_p =
          ecma_op_object_get_property_names(prop_obj_p, true, true, false);

      ecma_value_t *ecma_value_p = ecma_collection_iterator_init(props_p);
      uint32_t count = 0;
      while (ecma_value_p != NULL) {
        ecma_value_t object_prop_var = vm_op_get_value(current_arg, *ecma_value_p);
        if (ECMA_IS_VALUE_ERROR(object_prop_var)) {
          ecma_free_values_collection(props_p, 0);
          ret->var = object_prop_var;
          return;
        }

        args[actural_index] = object_prop_var;
        ecma_fast_free_value(object_prop_var);
        ecma_value_p = ecma_collection_iterator_next(ecma_value_p);
        count++;
        actural_index++;
      }
      QKING_ASSERT(count == props_p->item_count);
      ecma_free_values_collection(props_p, 0);
    } else {
      args[actural_index] = current_arg;
      actural_index++;
    }
  }

  switch (frame_ctx_p->call_operation) {
    case VM_EXEC_CALL: {
      completion_value =
          ecma_op_function_call(func_obj_p, this_value, args, actural_argc);
      break;
    }
    case VM_EXEC_CONSTRUCT: {
      completion_value =
          ecma_op_function_construct(func_obj_p, this_value, args, actural_argc);
      break;
    }
    default:
      break;
  }
  if (completion_value != ECMA_VALUE_EMPTY) {
    ecma_fast_free_value(ret->var);
    ret->var = completion_value;
  }
}

static void vm_sync_closure(ecma_register_t *register_p) {
  ecma_closure_t *closure_p = register_p->closure_p;
  if (closure_p) {
      ecma_value_t var = register_p->var;
      ecma_free_value_if_not_object(closure_p->var);
      closure_p->var = ecma_copy_value_if_not_object(var);
      ecma_register_t *register_current_p = closure_p->register_p;
      while (register_current_p) {
          if (register_current_p->var != var) {
              ecma_fast_free_value(register_current_p->var);
              register_current_p->var = ecma_fast_copy_value(var);
          }
          if (!register_current_p->next_p) {
              break;
          }
          register_current_p = register_current_p->next_p;
      }
  }
}
#define CHECK_COMPLETION_VALUE(test_value)          \
  if (ECMA_IS_VALUE_ERROR((test_value))) {          \
    ecma_fast_free_value(completion_value);         \
    completion_value = (test_value);                \
    (test_value) = ECMA_VALUE_UNDEFINED;            \
    goto ret_value_handle_label;                    \
  }

static ecma_value_t vm_run_loop(
    vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;
  const ecma_compiled_code_t *bytecode_header_p =
      frame_ctx_p->bytecode_header_p;
  const ecma_compiled_function_state_t *func_state_p =
      (ecma_compiled_function_state_t *)bytecode_header_p;
  bool is_strict = ((frame_ctx_p->bytecode_header_p->status_flags &
                     ECMA_CODE_FLAGS_STRICT_MODE) != 0);
  const ecma_value_t constants = func_state_p->constants;
  if (func_state_p->in_closure > 0) {
      ecma_op_function_load_register_in_closure(frame_ctx_p, func_state_p);
  }
  ecma_register_t *a = NULL;
  ecma_register_t *b = NULL;
  ecma_register_t *c = NULL;
  ecma_value_t *context_top_p = frame_ctx_p->exec_context_p;
  Instruction *pc = func_state_p->pc;
  Instruction *const pc_end = func_state_p->pc + func_state_p->pcc;
  ecma_register_t *const registers_p = frame_ctx_p->registers_p;
  uint32_t childrens = func_state_p->childrens;
  while (true) {
    while (pc != pc_end) {
      Instruction instruction = *pc++;
      OPCode op = (GET_OP_CODE(instruction));
#ifdef DEBUG_LOG_ENABLE
      qking_oputil_print_code(instruction, pc - func_state_p->pc - 1);
#endif
      switch (op) {
        case OP_MOVE: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_value_t lhs = a->var;
          ecma_value_t rhs = b->var;
          ecma_fast_free_value(lhs);
          a->var = ecma_fast_copy_value(rhs);
          vm_sync_closure(a);
          break;
        }
        case OP_LOADK: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          ecma_value_t constant =
              qking_get_property_by_index(constants, idxB);
          CHECK_COMPLETION_VALUE(constant)
          if (a->var != constant) {
              ecma_fast_free_value(a->var);
              a->var = ecma_fast_copy_value(constant);
          }
          ecma_fast_free_value(constant);
          break;
        }
        case OP_LOAD_NULL: {
          int idxA = (int)(GET_ARG_A(instruction));
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);
          a->var = ECMA_VALUE_NULL;
          break;
        }
        case OP_LOAD_UNDEFINED: {
          int idxA = (int)(GET_ARG_A(instruction));
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);
          a->var = ECMA_VALUE_UNDEFINED;
          break;
        }
        case OP_GET_GLOBAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          ecma_value_t prop_name_val =
              qking_get_property_by_index(constants, idxB);
          CHECK_COMPLETION_VALUE(prop_name_val)
          ecma_object_t *global_obj = ecma_builtin_get_global();
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);

#ifdef DEBUG_LOG_ENABLE
          QKING_TO_LOG_STR_START(debug_print_str, prop_name_val);
          LOGD("-- Get Global: %s",debug_print_str);
          QKING_TO_LOG_STR_FINISH(debug_print_str);
#endif
          a->var = ecma_op_object_get(
              global_obj, ecma_get_string_from_value(prop_name_val));
          if (a->var == ECMA_VALUE_UNDEFINED) {
#ifdef QKING_ENABLE_ERROR_MESSAGES
            ecma_value_t name_val = prop_name_val;
            ecma_fast_free_value(completion_value);
            completion_value = ecma_raise_standard_error_with_format(
                ECMA_ERROR_REFERENCE, "% is not defined", name_val);
#else  /* !QKING_ENABLE_ERROR_MESSAGES */
            completion_value = ecma_raise_reference_error(NULL);
#endif /* QKING_ENABLE_ERROR_MESSAGES */
            ecma_fast_free_value(prop_name_val);
            goto ret_value_handle_label;
          }
          ecma_fast_free_value(prop_name_val);
          break;
        }
        case OP_GET_BY_THIS_OR_GLOBAL: {
          int idxA = (int)(GET_ARG_A(instruction));//[ret]
          int idxB = (int)(GET_ARG_B(instruction));//[this]
          int idxC = (int)(GET_ARG_C(instruction));//[const]

          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;

          ecma_value_t prop_name_val = c->var;
          CHECK_COMPLETION_VALUE(prop_name_val)

#ifdef DEBUG_LOG_ENABLE
          QKING_TO_LOG_STR_START(debug_print_str, prop_name_val);
          LOGD("-- Get By This Or Global: %s", debug_print_str);
          QKING_TO_LOG_STR_FINISH(debug_print_str);
#endif
          if (ecma_is_value_object(b->var)){
            ecma_object_t *this_obj = ecma_get_object_from_value(b->var);
            ecma_value_t from_this = ecma_op_object_get(
                this_obj, ecma_get_string_from_value(prop_name_val));
            CHECK_COMPLETION_VALUE(from_this);
            if (from_this != ECMA_VALUE_UNDEFINED){
              //has value;
              ecma_fast_free_value(a->var);
              a->var = from_this;
              break;
            } else {
              ecma_fast_free_value(from_this);
              //fall through, using global to search
            }
          }

          ecma_fast_free_value(a->var);

          ecma_object_t *global_obj = ecma_builtin_get_global();

          a->var = ecma_op_object_get(
              global_obj, ecma_get_string_from_value(prop_name_val));
//          if (a->var == ECMA_VALUE_UNDEFINED) {
//#ifdef QKING_ENABLE_ERROR_MESSAGES
//            ecma_value_t name_val = prop_name_val;
//            completion_value = ecma_raise_standard_error_with_format(
//                ECMA_ERROR_REFERENCE, "% is not defined", name_val);
//#else  /* !QKING_ENABLE_ERROR_MESSAGES */
//            completion_value = ecma_raise_reference_error(NULL);
//#endif /* QKING_ENABLE_ERROR_MESSAGES */
//            goto ret_value_handle_label;
//          }
          break;
        }
        case OP_GET_LOCAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          const ecma_value_t prop_name_val =
              qking_get_property_by_index(constants, idxB);
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);
          a->var = ecma_op_get_binding_value(
              frame_ctx_p->lex_env_p, ecma_get_string_from_value(prop_name_val),
              false);
          ecma_fast_free_value(prop_name_val);
          break;
        }
        case OP_OUT_CLOSURE: {
          int idxA = (int)(GET_ARG_Ax(instruction));
          a = registers_p + idxA;
#ifdef QKING_ENABLE_GC_DEBUG
          printf("[gc][%d]=>[add_out_closure][register][%d]\n", a->var, idxA);
#endif
          ecma_op_function_add_out_closure(a);
          break;
        }
        case OP_GET_PROPERTY: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_value_t result = vm_op_get_value(lhs, rhs);
          CHECK_COMPLETION_VALUE(result);
          ecma_fast_free_value(a->var);
          a->var = result;
          break;
        }
        case OP_SET_PROPERTY: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t property = b->var;
          ecma_value_t var = c->var;
          ecma_value_t result;
          if (ecma_is_value_object(property) && ecma_is_value_object(var)) {
            result = vm_op_set_object_keys_value(&a->var, property, is_strict);
          } else {
            result = vm_op_set_value(&a->var, property, var, is_strict);
          }
          CHECK_COMPLETION_VALUE(result);
          break;
        }
        case OP_CALL:
        case OP_CONSTRUCTOR: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          uint32_t argc = (uint32_t)(idxB);
          c = registers_p + idxC;
          if (!ecma_op_is_callable(c->var)) {
            ecma_fast_free_value(a->var);
            a->var = ecma_raise_type_error(ECMA_ERR_MSG("Expected a function."));
          } else {
            switch (op) {
              case OP_CALL: {
                frame_ctx_p->call_operation = VM_EXEC_CALL;
                break;
              }
              case OP_CONSTRUCTOR: {
                frame_ctx_p->call_operation = VM_EXEC_CONSTRUCT;
                break;
              }
              default:
                frame_ctx_p->call_operation = VM_NO_EXEC_OP;
                break;
            }
            LOGD("\n======================> FUNC START");
            ecma_func_call(frame_ctx_p, c, argc, a);
            LOGD("FUNC RETURN <=====================\n");
          }
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_NEW: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          ecma_value_t new_obj = ECMA_VALUE_UNDEFINED;
          switch (idxB) {
            case VM_OP_NEW_TYPE_MAP: {
              new_obj = ecma_op_map_create(NULL, 0);
              break;
            }
            case VM_OP_NEW_TYPE_ARRAY: {
              new_obj = ecma_op_create_array_object(NULL, 0, false);
              if (idxC > 0) {
                ecma_object_t *array_obj_p =
                    ecma_get_object_from_value(new_obj);
                ecma_extended_object_t *ext_array_obj_p =
                    (ecma_extended_object_t *)array_obj_p;
                ext_array_obj_p->u.array.length = idxC;
              }
              break;
            }
            case VM_OP_NEW_TYPE_FUNCTION: {
              if (idxC >= childrens) {
                completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("load func error"));
                goto ret_value_handle_label;
              }
              ecma_compiled_function_state_t *func_state_current_p =
                  func_state_p->pp_childrens[idxC];
              ecma_compiled_code_t *bytecode_p = &func_state_current_p->header;
              ecma_object_t *func_obj_p = NULL;
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
              if (!(bytecode_p->status_flags &
                    ECMA_CODE_FLAGS_ARROW_FUNCTION)) {
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
                func_obj_p = ecma_op_create_function_object(
                    frame_ctx_p->lex_env_p, bytecode_p);
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
              } else {
                ecma_value_t this_binding = registers_p[0].var;
                func_obj_p = ecma_op_create_arrow_function_object(
                    frame_ctx_p->lex_env_p, bytecode_p,
                    this_binding);
              }
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
              if (func_state_current_p->in_closure > 0) {
                  ecma_op_function_load_in_closure(frame_ctx_p, func_obj_p, func_state_current_p);
              }
              new_obj = ecma_make_object_value(func_obj_p);
              break;
            }
            case VM_OP_NEW_TYPE_IMPL_CONSTRUCTOR: {
              ecma_object_t *function_obj_p = vm_op_new_constructor_object();
              new_obj = ecma_make_object_value(function_obj_p);
              break;
            }
            case VM_OP_NEW_TYPE_IMPL_SUPER_CONSTRUCTOR: {
              c = registers_p + idxC;
              ecma_value_t super_value = c->var;
              ecma_object_t *current_constructor_obj_p =
                  vm_op_new_constructor_object();
              uint16_t type_flags_refs =
                  current_constructor_obj_p->type_flags_refs;
              const int new_type = ECMA_OBJECT_TYPE_BOUND_FUNCTION -
                                   ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION;
              current_constructor_obj_p->type_flags_refs =
                  (uint16_t)(type_flags_refs + new_type);
              ecma_extended_object_t *ext_function_p =
                  (ecma_extended_object_t *)current_constructor_obj_p;
              ecma_object_t *super_obj_p =
                  ecma_get_object_from_value(super_value);
              ECMA_SET_INTERNAL_VALUE_POINTER(
                  ext_function_p->u.bound_function.target_function,
                  super_obj_p);
              ext_function_p->u.bound_function.args_len_or_this =
                  ECMA_VALUE_IMPLICIT_CONSTRUCTOR;
              new_obj = ecma_make_object_value(current_constructor_obj_p);
              break;
            }
            case VM_OP_NEW_TYPE_CONSTRUCTOR: {
              ecma_object_t *function_obj_p = vm_op_new_constructor_object();
              ecma_extended_object_t *ext_func_obj_p =
                  (ecma_extended_object_t *)function_obj_p;
              if (idxC >= childrens) {
                completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("load func error"));
                goto ret_value_handle_label;
              }
              ecma_compiled_function_state_t *func_state_current_p =
                  func_state_p->pp_childrens[idxC];
              ecma_compiled_code_t *bytecode_p = &func_state_current_p->header;
              uint16_t type_flags_refs = function_obj_p->type_flags_refs;
              const int new_type = ECMA_OBJECT_TYPE_FUNCTION -
                                   ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION;
              function_obj_p->type_flags_refs =
                  (uint16_t)(type_flags_refs + new_type);
              bytecode_p->status_flags |= ECMA_CODE_FLAGS_CONSTRUCTOR;
              ecma_bytecode_ref((ecma_compiled_code_t *)bytecode_p);
              ECMA_SET_INTERNAL_VALUE_POINTER(
                  ext_func_obj_p->u.function.bytecode_cp, bytecode_p);
              ECMA_SET_INTERNAL_VALUE_POINTER(
                  ext_func_obj_p->u.function.scope_cp, frame_ctx_p->lex_env_p);
                if (func_state_current_p->in_closure > 0) {
                    ecma_op_function_load_in_closure(frame_ctx_p, function_obj_p, func_state_current_p);
                }
              new_obj = ecma_make_object_value(function_obj_p);
              break;
            }
            case VM_OP_NEW_TYPE_OBJECT: {
              ecma_object_t *prototype_p =
                  ecma_builtin_get(ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
              ecma_object_t *obj_p =
                  ecma_create_object(prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);
              ecma_deref_object(prototype_p);
              new_obj = ecma_make_object_value(obj_p);
              break;
            }
            default:
              break;
          }
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);
          a->var = new_obj;
          vm_sync_closure(a);
          break;
        }
        case OP_INHERITANCE: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          LOGD("vm_op_inherit %i %i", a->var, b->var);
          ecma_value_t inherit_result = vm_op_inherit(frame_ctx_p, a->var, b->var);
          CHECK_COMPLETION_VALUE(inherit_result);
          break;
        }
        case OP_GET_SUPER: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          ecma_fast_free_value(a->var);
          switch (idxB) {
            case VM_OP_GET_SUPER_TYPE_CONSTRUCTOR: {
              ecma_object_t *child_class_p =
                  ecma_get_object_from_value(frame_ctx_p->this_function);
              ecma_object_t *super_class_p = ECMA_GET_POINTER(
                  ecma_object_t,
                  child_class_p->prototype_or_outer_reference_cp);
              a->var =
                  ecma_fast_copy_value(ecma_make_object_value(super_class_p));
              break;
            }
            case VM_OP_GET_SUPER_TYPE_PROTOTYPE: {
              ecma_object_t *child_prototype_p = ecma_get_object_prototype(
                  ecma_get_object_from_value(registers_p[0].var));
              if (child_prototype_p == NULL) {
                completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("can't get child prototype with OP_GET_SUPER"));
                goto ret_value_handle_label;
              }
              ecma_object_t *super_prototype_p =
                  ecma_get_object_prototype(child_prototype_p);
              if (super_prototype_p == NULL) {
                completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("can't get super prototype with OP_GET_SUPER"));
                goto ret_value_handle_label;
              }
              ecma_value_t super_prototype =
                  ecma_make_object_value(super_prototype_p);
              a->var = ecma_fast_copy_value(super_prototype);
              break;
            }
            default:
              completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("unknown get super type error with OP_GET_SUPER"));
              goto ret_value_handle_label;
          }
          break;
        }
        case OP_N_JMP: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          if (!ecma_op_to_boolean(a->var)) {
            pc += idxB;
          }
          break;
        }
        case OP_JMP_PC: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          if (ecma_op_to_boolean(a->var)) {
            pc = func_state_p->pc + idxB;
          }
          break;
        }
        case OP_GOTO: {
          int idxA = (int)(GET_ARG_Ax(instruction));
          pc = func_state_p->pc + idxA;
          break;
        }
        case OP_EQUAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_fast_free_value(a->var);
          a->var = opfunc_equality(b->var, c->var);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_STRICT_EQUAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          bool is_equal = ecma_op_strict_equality_compare(b->var, c->var);
          ecma_fast_free_value(a->var);
          a->var = ecma_make_boolean_value(is_equal);
          break;
        }
        case OP_INSTANCEOF: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_fast_free_value(a->var);
          a->var = opfunc_instanceof(b->var, c->var);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_NOT: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_fast_free_value(a->var);
          a->var = opfunc_logical_not(b->var);
          break;
        }
        case OP_TYPEOF: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_fast_free_value(a->var);
          a->var = opfunc_typeof(b->var);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_ADD: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var =
                ecma_make_int32_value((int32_t)(left_integer + right_integer));
            break;
          }
          if (ecma_is_value_float_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t new_value = (ecma_get_float_from_value(lhs) +
                                       ecma_get_number_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          if (ecma_is_value_float_number(rhs) &&
              ecma_is_value_integer_number(lhs)) {
            ecma_number_t new_value =
                ((ecma_number_t)ecma_get_integer_from_value(lhs) +
                 ecma_get_float_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          a->var = opfunc_addition(lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_SUB: {
          QKING_STATIC_ASSERT(ECMA_INTEGER_NUMBER_MAX * 2 <= INT32_MAX &&
                                  ECMA_INTEGER_NUMBER_MIN * 2 >= INT32_MIN,
                              doubled_ecma_numbers_must_fit_into_int32_range);
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_ASSERT(!ECMA_IS_VALUE_ERROR(lhs) && !ECMA_IS_VALUE_ERROR(rhs));
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var =
                ecma_make_int32_value((int32_t)(left_integer - right_integer));
            break;
          }
          if (ecma_is_value_float_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t new_value = (ecma_get_float_from_value(lhs) -
                                       ecma_get_number_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          if (ecma_is_value_float_number(rhs) &&
              ecma_is_value_integer_number(lhs)) {
            ecma_number_t new_value =
                ((ecma_number_t)ecma_get_integer_from_value(lhs) -
                 ecma_get_float_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          a->var =
              do_number_arithmetic(NUMBER_ARITHMETIC_SUBSTRACTION, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_MUL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_ASSERT(!ECMA_IS_VALUE_ERROR(lhs) && !ECMA_IS_VALUE_ERROR(rhs));
          QKING_STATIC_ASSERT(
              ECMA_INTEGER_MULTIPLY_MAX * ECMA_INTEGER_MULTIPLY_MAX <=
                      ECMA_INTEGER_NUMBER_MAX &&
                  -(ECMA_INTEGER_MULTIPLY_MAX * ECMA_INTEGER_MULTIPLY_MAX) >=
                      ECMA_INTEGER_NUMBER_MIN,
              square_of_integer_multiply_max_must_fit_into_integer_value_range);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            if (-ECMA_INTEGER_MULTIPLY_MAX <= left_integer &&
                left_integer <= ECMA_INTEGER_MULTIPLY_MAX &&
                -ECMA_INTEGER_MULTIPLY_MAX <= right_integer &&
                right_integer <= ECMA_INTEGER_MULTIPLY_MAX && lhs != 0 &&
                rhs != 0) {
              a->var = ecma_integer_multiply(left_integer, right_integer);
              break;
            }
            ecma_number_t multiply =
                (ecma_number_t)left_integer * (ecma_number_t)right_integer;
            a->var = ecma_make_number_value(multiply);
            break;
          }
          if (ecma_is_value_float_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t new_value = (ecma_get_float_from_value(lhs) *
                                       ecma_get_number_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          if (ecma_is_value_float_number(rhs) &&
              ecma_is_value_integer_number(lhs)) {
            ecma_number_t new_value =
                ((ecma_number_t)ecma_get_integer_from_value(lhs) *
                 ecma_get_float_from_value(rhs));
            a->var = ecma_make_number_value(new_value);
            break;
          }
          a->var =
              do_number_arithmetic(NUMBER_ARITHMETIC_MULTIPLICATION, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_DIV: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_ASSERT(!ECMA_IS_VALUE_ERROR(lhs) && !ECMA_IS_VALUE_ERROR(rhs));
          a->var = do_number_arithmetic(NUMBER_ARITHMETIC_DIVISION, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_MOD: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_ASSERT(!ECMA_IS_VALUE_ERROR(lhs) && !ECMA_IS_VALUE_ERROR(rhs));
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);

            if (right_integer != 0) {
              ecma_integer_value_t mod_result = left_integer % right_integer;

              if (mod_result != 0 || left_integer >= 0) {
                a->var = ecma_make_integer_value(mod_result);
                break;
              }
            }
          }
          a->var = do_number_arithmetic(NUMBER_ARITHMETIC_REMAINDER, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_LESS: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            bool is_less =
                (ecma_integer_value_t)lhs < (ecma_integer_value_t)rhs;
            a->var = ecma_make_boolean_value(is_less);
            break;
          }
          if (ecma_is_value_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t left_number = ecma_get_number_from_value(lhs);
            ecma_number_t right_number = ecma_get_number_from_value(rhs);
            a->var = ecma_make_boolean_value(left_number < right_number);
            break;
          }
          a->var = opfunc_relation(lhs, rhs, true, false);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_GREATER: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer = (ecma_integer_value_t)lhs;
            ecma_integer_value_t right_integer = (ecma_integer_value_t)rhs;
            a->var = ecma_make_boolean_value(left_integer > right_integer);
            break;
          }
          if (ecma_is_value_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t left_number = ecma_get_number_from_value(lhs);
            ecma_number_t right_number = ecma_get_number_from_value(rhs);
            a->var = ecma_make_boolean_value(left_number > right_number);
            break;
          }
          a->var = opfunc_relation(lhs, rhs, false, false);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_LESS_EQUAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer = (ecma_integer_value_t)lhs;
            ecma_integer_value_t right_integer = (ecma_integer_value_t)rhs;
            a->var = ecma_make_boolean_value(left_integer <= right_integer);
            break;
          }
          if (ecma_is_value_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t left_number = ecma_get_number_from_value(lhs);
            ecma_number_t right_number = ecma_get_number_from_value(rhs);
            a->var = ecma_make_boolean_value(left_number <= right_number);
            break;
          }
          a->var = opfunc_relation(lhs, rhs, false, true);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_GREATER_EQUAL: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer = (ecma_integer_value_t)lhs;
            ecma_integer_value_t right_integer = (ecma_integer_value_t)rhs;
            a->var = ecma_make_boolean_value(left_integer >= right_integer);
            break;
          }
          if (ecma_is_value_number(lhs) && ecma_is_value_number(rhs)) {
            ecma_number_t left_number = ecma_get_number_from_value(lhs);
            ecma_number_t right_number = ecma_get_number_from_value(rhs);
            a->var = ecma_make_boolean_value(left_number >= right_number);
            break;
          }
          a->var = opfunc_relation(lhs, rhs, true, true);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_PRE_INCR:
        case OP_PRE_DECR:
        case OP_POST_INCR:
        case OP_POST_DECR: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_value_t lhs = a->var;
          ecma_value_t ret = ECMA_VALUE_EMPTY;
          bool is_post =
              (op == OP_POST_INCR || op == OP_POST_DECR) ? true : false;
          if (is_post) {
            ecma_fast_free_value(b->var);
            b->var = ecma_fast_copy_value(lhs);
          }
          bool use_self = false;
          if (ecma_is_value_integer_number(lhs)) {
            ecma_integer_value_t int_value = (ecma_integer_value_t)lhs;
            ecma_integer_value_t int_increase = 0;
            if (op == OP_POST_DECR || op == OP_PRE_DECR) {
              if (int_value > ECMA_INTEGER_NUMBER_MIN_SHIFTED) {
                int_increase = -(1 << ECMA_DIRECT_SHIFT);
              }
            } else if (int_value < ECMA_INTEGER_NUMBER_MAX_SHIFTED) {
              int_increase = 1 << ECMA_DIRECT_SHIFT;
            }
            if (QKING_LIKELY(int_increase != 0)) {
              ecma_fast_free_value(a->var);
              a->var = (ecma_value_t)(int_value + int_increase);
              vm_sync_closure(a);
              if (!is_post) {
                ecma_fast_free_value(b->var);
                b->var = ecma_fast_copy_value(a->var);
              }
              break;
            }
          } else if (ecma_is_value_float_number(lhs)) {
            ret = lhs;
            use_self = true;
          } else {
            ret = ecma_op_to_number(lhs);
            CHECK_COMPLETION_VALUE(ret);
          }
          ecma_number_t increase = ECMA_NUMBER_ONE;
          ecma_number_t result_number = ecma_get_number_from_value(ret);
          if (op == OP_POST_DECR || op == OP_PRE_DECR) {
            /* For decrement operators */
            increase = ECMA_NUMBER_MINUS_ONE;
          }
          if (ecma_is_value_integer_number(ret)) {
            ecma_fast_free_value(a->var);
            a->var = ecma_make_number_value(result_number + increase);
          } else {
            if (!use_self) {
              ecma_fast_free_value(a->var);
            }
            a->var = ecma_update_float_number(ret, result_number + increase);
          }
          vm_sync_closure(a);
          if (!is_post) {
            ecma_fast_free_value(b->var);
            b->var = ecma_fast_copy_value(a->var);
          }
          break;
        }
        case OP_BIT_OR: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, lhs)) {
            a->var = lhs | rhs;
            break;
          }
          a->var = do_number_bitwise_logic(NUMBER_BITWISE_LOGIC_OR, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_BIT_XOR: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            a->var = (lhs ^ rhs) & (ecma_value_t)(~ECMA_DIRECT_TYPE_MASK);
            break;
          }
          a->var = do_number_bitwise_logic(NUMBER_BITWISE_LOGIC_XOR, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_BIT_AND: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            a->var = lhs & rhs;
            break;
          }
          a->var = do_number_bitwise_logic(NUMBER_BITWISE_LOGIC_AND, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_BIT_NOT: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_value_t rhs = b->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_is_value_integer_number(rhs)) {
            a->var = (~rhs) & (ecma_value_t)(~ECMA_DIRECT_TYPE_MASK);
            break;
          }
          a->var = do_number_bitwise_logic(NUMBER_BITWISE_NOT, rhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_LEFT_SHIFT: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var = ecma_make_int32_value(
                (int32_t)(left_integer << (right_integer & 0x1f)));
            break;
          }
          a->var = do_number_bitwise_logic(NUMBER_BITWISE_SHIFT_LEFT, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_RIGHT_SHIFT: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var =
                ecma_make_integer_value(left_integer >> (right_integer & 0x1f));
            break;
          }
          a->var =
              do_number_bitwise_logic(NUMBER_BITWISE_SHIFT_RIGHT, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_ZERO_RIGHT_SHIFT: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            ecma_integer_value_t left_integer =
                ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var =
                ecma_make_integer_value(left_integer >> (right_integer & 0x1f));
            break;
          }
          a->var =
              do_number_bitwise_logic(NUMBER_BITWISE_SHIFT_URIGHT, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_UNS: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          QKING_STATIC_ASSERT(
              ECMA_DIRECT_TYPE_MASK == ((1 << ECMA_DIRECT_SHIFT) - 1),
              direct_type_mask_must_fill_all_bits_before_the_value_starts);
          if (ecma_are_values_integer_numbers(lhs, rhs)) {
            uint32_t left_uint32 = (uint32_t)ecma_get_integer_from_value(lhs);
            ecma_integer_value_t right_integer =
                ecma_get_integer_from_value(rhs);
            a->var =
                ecma_make_uint32_value(left_uint32 >> (right_integer & 0x1f));
            break;
          }
          a->var =
              do_number_bitwise_logic(NUMBER_BITWISE_SHIFT_URIGHT, lhs, rhs);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_RETURN0: {
          ecma_fast_free_value(completion_value);
          completion_value = ECMA_VALUE_UNDEFINED;
          goto ret_value_handle_label;
        }
        case OP_RETURN1: {
          int idxA = (int)(GET_ARG_Ax(instruction));
          a = registers_p + idxA;
          ecma_fast_free_value(completion_value);
          completion_value = ecma_fast_copy_value(a->var);
          goto ret_value_handle_label;
        }
        case OP_THROW: {
          int idxA = (int)(GET_ARG_Ax(instruction));
          a = registers_p + idxA;
          ecma_fast_free_value(QKING_CONTEXT(error_value));
          QKING_CONTEXT(error_value) = ecma_fast_copy_value(a->var);
          QKING_CONTEXT(status_flags) |= ECMA_STATUS_EXCEPTION;
          ecma_fast_free_value(completion_value);
          completion_value = ECMA_VALUE_ERROR;
          goto ret_value_handle_label;
        }
        case OP_TRY: {
          uintptr_t next_offset =
              (uintptr_t)(GET_ARG_Ax(instruction));
          uintptr_t branch_offset =
              pc - 1 - func_state_p->pc +
              next_offset;  // PC already ++ in loop begin.
          VM_PLUS_EQUAL_U16(frame_ctx_p->context_depth,
                            ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
          context_top_p += ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;

          context_top_p[-1] =
              (ecma_value_t)VM_CREATE_CONTEXT(VM_CONTEXT_TRY, branch_offset);
          break;
        }
        case OP_CATCH: {
          QKING_ASSERT(VM_GET_CONTEXT_TYPE(context_top_p[-1]) ==
                       VM_CONTEXT_TRY);
          uintptr_t next_offset =
              (uintptr_t)(GET_ARG_Bx(instruction));
          pc += next_offset - 1;  // PC already ++ in loop begin.
          break;
        }
        case OP_FINALLY: {
          uintptr_t next_offset =
              (uintptr_t)(GET_ARG_Ax(instruction));
          uintptr_t branch_offset = pc - 1 - func_state_p->pc + next_offset;

          QKING_ASSERT(
              VM_GET_CONTEXT_TYPE(context_top_p[-1]) == VM_CONTEXT_TRY ||
              VM_GET_CONTEXT_TYPE(context_top_p[-1]) == VM_CONTEXT_CATCH);

          if (VM_GET_CONTEXT_TYPE(context_top_p[-1]) == VM_CONTEXT_CATCH) {
            ecma_object_t *lex_env_p = frame_ctx_p->lex_env_p;
            frame_ctx_p->lex_env_p =
                ecma_get_lex_env_outer_reference(lex_env_p);
            ecma_deref_object(lex_env_p);
          }

          context_top_p[-1] = (ecma_value_t)VM_CREATE_CONTEXT(
              VM_CONTEXT_FINALLY_JUMP, branch_offset);
          context_top_p[-2] = (ecma_value_t)branch_offset;
          break;
        }
        case OP_POP_CONTEXT: {
          uint8_t type = (uint8_t)(GET_ARG_A(instruction));
          if (type == VM_OP_POP_CONTEXT_TYPE_END) {
            switch (VM_GET_CONTEXT_TYPE(context_top_p[-1])) {
              case VM_CONTEXT_FINALLY_JUMP: {
                uint32_t jump_target = context_top_p[-2];

                VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                                   ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
                context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;

                if (vm_stack_find_finally(frame_ctx_p, &context_top_p,
                                          VM_CONTEXT_FINALLY_JUMP,
                                          jump_target)) {
                  QKING_ASSERT(VM_GET_CONTEXT_TYPE(context_top_p[-1]) ==
                               VM_CONTEXT_FINALLY_JUMP);
                  pc = frame_ctx_p->pc_current_p;
                  context_top_p[-2] = jump_target;
                } else {
                  pc = frame_ctx_p->pc_start_p + jump_target;
                }
                break;
              }
              case VM_CONTEXT_FINALLY_THROW: {
                ecma_fast_free_value(QKING_CONTEXT(error_value));
                QKING_CONTEXT(error_value) = context_top_p[-2];
                QKING_CONTEXT(status_flags) |= ECMA_STATUS_EXCEPTION;

                VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                                   ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
                context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
                ecma_fast_free_value(completion_value);
                completion_value = ECMA_VALUE_ERROR;

                goto ret_value_handle_label;
              }
              case VM_CONTEXT_FINALLY_RETURN: {
                ecma_fast_free_value(completion_value);
                completion_value = context_top_p[-2];

                VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                                   ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
                context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
                goto ret_value_handle_label;
              }
              default: {
                context_top_p =
                    vm_stack_context_abort(frame_ctx_p, context_top_p);
              }
            }
          } else if (type == VM_OP_POP_CONTEXT_TYPE_JUMP) {
            uintptr_t next_offset =
                (uintptr_t)(GET_ARG_Bx(instruction));

            if (vm_stack_find_finally(frame_ctx_p, &context_top_p,
                                      VM_CONTEXT_FINALLY_JUMP,
                                      (uint32_t)next_offset)) {
              QKING_ASSERT(VM_GET_CONTEXT_TYPE(context_top_p[-1]) ==
                           VM_CONTEXT_FINALLY_JUMP);
              pc = frame_ctx_p->pc_current_p;
              context_top_p[-2] = (uint32_t)next_offset;
            } else {
              pc = frame_ctx_p->pc_start_p + next_offset;
            }

            continue;
          } else {
            QKING_UNREACHABLE();
          }
          break;
        }
        case OP_FOR_IN: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_fast_free_value(a->var);
          a->var = vm_op_for_in(&b->var, c->var);
          break;
        }
        case OP_IN: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
          ecma_fast_free_value(a->var);
          a->var = opfunc_in(lhs, rhs);
          break;
        }
        case OP_DELETE: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_Bx(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          ecma_fast_free_value(a->var);
          a->var = vm_op_delete_var(b->var, frame_ctx_p->lex_env_p);
          CHECK_COMPLETION_VALUE(a->var);
          break;
        }
        case OP_DELETE_PROPERTY: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t lhs = b->var;
          ecma_value_t rhs = c->var;
//          LOGD("lhs:%s\n", convert_to_log_string_from_value(lhs).c_str());
//          LOGD("rhs:%s\n", convert_to_log_string_from_value(rhs).c_str());
          ecma_value_t result = vm_op_delete_prop(lhs, rhs, is_strict);
          CHECK_COMPLETION_VALUE(result)
          QKING_ASSERT(ecma_is_value_boolean(result));
          ecma_fast_free_value(a->var);
          a->var = result;
          break;
        }
        case OP_SET_GETTER:
        case OP_SET_SETTER: {
          int idxA = (int)(GET_ARG_A(instruction));
          int idxB = (int)(GET_ARG_B(instruction));
          int idxC = (int)(GET_ARG_C(instruction));
          a = registers_p + idxA;
          b = registers_p + idxB;
          c = registers_p + idxC;
          ecma_value_t left_value = b->var;
          ecma_value_t right_value = c->var;
          ecma_value_t result = left_value;
          if (QKING_UNLIKELY(!ecma_is_value_string(left_value))) {
            result = ecma_op_to_string(left_value);
            CHECK_COMPLETION_VALUE(result);
          }
          ecma_string_t *prop_name_p = ecma_get_string_from_value(result);
          vm_op_set_accessor(op == OP_SET_GETTER, a->var, prop_name_p,
                             right_value);
          if (!ecma_is_value_string(left_value)) {
            ecma_deref_ecma_string(prop_name_p);
          }
          break;
        }
        case OP_INVALID: {
          ecma_fast_free_value(completion_value);
          completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("VM op_invalid"));
          goto ret_value_handle_label;
        }
        default: {
          ecma_fast_free_value(completion_value);
          completion_value = ecma_raise_common_error (ECMA_ERR_MSG ("VM op unknown"));
          goto ret_value_handle_label;
        }
      }  // opcode switch

    }  // opcode while loop
    // op_code loops to an end.
    QKING_ASSERT(pc == pc_end);
    return ECMA_VALUE_UNDEFINED;
  ret_value_handle_label:
    if (frame_ctx_p->context_depth == 0) {
      /* In most cases there is no context. */
      //如果有error, 那么上一层的frame也会判断的.
      return completion_value;
    }

    if (!ECMA_IS_VALUE_ERROR(completion_value)) {
      //普通值的情况. return
      if (vm_stack_find_finally(frame_ctx_p, &context_top_p,
                                VM_CONTEXT_FINALLY_RETURN, 0)) {
        QKING_ASSERT(VM_GET_CONTEXT_TYPE(context_top_p[-1]) ==
                     VM_CONTEXT_FINALLY_RETURN);

        pc = frame_ctx_p->pc_current_p;
        context_top_p[-2] = completion_value;
        continue;
      }
    } else if (QKING_CONTEXT(status_flags) & ECMA_STATUS_EXCEPTION) {
      if (vm_stack_find_finally(frame_ctx_p, &context_top_p,
                                VM_CONTEXT_FINALLY_THROW, 0)) {
        pc = frame_ctx_p->pc_current_p;

        if (VM_GET_CONTEXT_TYPE(context_top_p[-1]) == VM_CONTEXT_CATCH) {

          QKING_ASSERT(GET_OP_CODE(pc[-1]) == OP_CATCH);
          ecma_register_t *register_error = registers_p + (uint32_t)(GET_ARG_A(pc[-1]));
          ecma_fast_free_value(register_error->var);
          register_error->var = ecma_copy_value(QKING_CONTEXT(error_value));
          ecma_object_t *catch_env_p = ecma_create_decl_lex_env(frame_ctx_p->lex_env_p);
          frame_ctx_p->lex_env_p = catch_env_p;
        } else {
          QKING_ASSERT(VM_GET_CONTEXT_TYPE(context_top_p[-1]) ==
                       VM_CONTEXT_FINALLY_THROW);
          context_top_p[-2] = QKING_CONTEXT(error_value);
        }
        
        continue;
      }
    } else {
      do {
        context_top_p = vm_stack_context_abort(frame_ctx_p, context_top_p);
      } while (frame_ctx_p->context_depth > 0);
    }

    return completion_value;
  }  // error while loop

  QKING_UNREACHABLE();

  //    return completion_value;

} /* vm_init_loop */

/**
 * Execute code block.
 *
 * @return ecma value
 */
static ecma_value_t QKING_ATTR_NOINLINE
vm_execute(vm_frame_ctx_t *frame_ctx_p, const ecma_value_t this_binding_value,
           const ecma_value_t *arg_p, ecma_length_t argc) {
  const ecma_compiled_code_t *bytecode_header_p =
      frame_ctx_p->bytecode_header_p;
  const ecma_compiled_function_state_t *func_state_p =
      (ecma_compiled_function_state_t *)bytecode_header_p;
  ecma_value_t completion_value;
  uint16_t argument_end = func_state_p->argc;
  uint16_t register_end = func_state_p->stack_size;
  if (argc > argument_end) {
    argc = argument_end;
  }
  frame_ctx_p->registers_p[0].var = ecma_fast_copy_value(this_binding_value);
  for (uint32_t i = 0; i < argc; i++) {
    frame_ctx_p->registers_p[i + 1].var = ecma_fast_copy_value(arg_p[i]);
  }
  /* The arg_list_len contains the end of the copied arguments.
   * Fill everything else with undefined. */
  if (register_end > argc) {
    ecma_register_t *stack_p = frame_ctx_p->registers_p + argc + 1;
    for (uint32_t i = argc + 1; i < register_end; i++) {
      (*stack_p++).var = ECMA_VALUE_UNDEFINED;
    }
  }
  QKING_CONTEXT(vm_top_context_p) = frame_ctx_p;

  completion_value = vm_run_loop(frame_ctx_p);

  /* Free arguments and registers */
    ecma_register_t *register_p = frame_ctx_p->registers_p;
  for (uint32_t i = 0; i < register_end; i++) {
      if (register_p[i].closure_p) {
#ifdef QKING_ENABLE_GC_DEBUG
          printf("[gc][%d]=>[remove][closure][register][%d]\n", register_p[i].var, i);
#endif
          ecma_op_function_remove_register_from_closure(&register_p[i]);
      }
      ecma_fast_free_value(register_p[i].var);
    //    printf("Free %d var:%i",i, frame_ctx_p->registers_p[i].var);
    //    qkingx_handler_value_print(frame_ctx_p->registers_p[i].var);
  }
#ifdef QKING_DEBUGGER
  if (QKING_CONTEXT(debugger_stop_context) == QKING_CONTEXT(vm_top_context_p)) {
    /* The engine will stop when the next breakpoint is reached. */
    QKING_ASSERT(QKING_CONTEXT(debugger_flags) & QKING_DEBUGGER_VM_STOP);
    QKING_CONTEXT(debugger_stop_context) = NULL;
  }
#endif /* QKING_DEBUGGER */

  QKING_CONTEXT(vm_top_context_p) = frame_ctx_p->prev_context_p;
  return completion_value;
} /* vm_execute */

/**
 * Run the code.
 *
 * @return ecma value
 */
ecma_value_t vm_run(
    const ecma_compiled_code_t *bytecode_header_p, /**< byte-code data header */
    ecma_value_t this_func_value,
    ecma_value_t this_binding_value, /**< value of 'ThisBinding' */
    ecma_object_t *lex_env_p,        /**< lexical environment to use */
    const ecma_value_t *arg_list_p,  /**< arguments list */
    ecma_length_t argc) /**< length of arguments list, this not included */
{
  do {
    size_t size = ((size_t)bytecode_header_p->size) << JMEM_ALIGNMENT_LOG;
    if (size !=
        QKING_ALIGNUP(sizeof(ecma_compiled_function_state_t), JMEM_ALIGNMENT)) {
      LOGW("WARNING!!: size != QKING_ALIGNUP(sizeof(ecma_compiled_function_state_t), JMEM_ALIGNMENT)");
//      QKING_UNREACHABLE();
      break;
    }
    vm_frame_ctx_t frame_ctx;
    memset(&frame_ctx, 0, sizeof(vm_frame_ctx_t));
    const ecma_compiled_function_state_t *func_state_p =
        (ecma_compiled_function_state_t *)bytecode_header_p;
    uint32_t call_stack_size = func_state_p ? func_state_p->stack_size : 0;
    uint32_t context_stack_size = func_state_p ? func_state_p->context_size : 0;
    frame_ctx.bytecode_header_p = bytecode_header_p;
    frame_ctx.lex_env_p = lex_env_p;
    frame_ctx.prev_context_p = QKING_CONTEXT(vm_top_context_p);
    frame_ctx.this_binding = this_binding_value;
#ifdef QKING_ENABLE_LINE_INFO
    frame_ctx.resource_name = ECMA_VALUE_UNDEFINED;
    frame_ctx.current_line = 0;
#endif /* QKING_ENABLE_LINE_INFO */
    frame_ctx.context_depth = 0;
    frame_ctx.call_operation = VM_NO_EXEC_OP;
    /* Use QKING_MAX() to avoid array declaration with size 0. */
    QKING_VLA(ecma_register_t, stack, QKING_MAX(call_stack_size, 1));
    memset(stack, 0, sizeof(ecma_register_t) * QKING_MAX(call_stack_size, 1));
    frame_ctx.registers_p = stack;
    QKING_VLA(ecma_value_t, context_stack, QKING_MAX(context_stack_size, 1));
    memset(context_stack, 0,
           sizeof(ecma_value_t) * QKING_MAX(context_stack_size, 1));
    frame_ctx.exec_context_p = context_stack;
    frame_ctx.this_function = this_func_value;
    frame_ctx.pc_current_p = func_state_p->pc;
    frame_ctx.pc_start_p = func_state_p->pc;
    frame_ctx.pc_count = func_state_p->pcc;
    return vm_execute(&frame_ctx, this_binding_value, arg_list_p, argc);

  } while (0);

  return ECMA_VALUE_UNDEFINED;
} /* vm_run */

/**
 * Abort (finalize) the current stack context, and remove it.
 *
 * @return new stack top
 */
ecma_value_t *vm_stack_context_abort(
    vm_frame_ctx_t *frame_ctx_p,    /**< frame context */
    ecma_value_t *vm_context_top_p) /**< current stack top */
{
  switch (VM_GET_CONTEXT_TYPE(vm_context_top_p[-1])) {
    case VM_CONTEXT_FINALLY_THROW:
    case VM_CONTEXT_FINALLY_RETURN: {
      ecma_free_value(vm_context_top_p[-2]);

      VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                         ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
      vm_context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
      break;
    }
    case VM_CONTEXT_FINALLY_JUMP:
    case VM_CONTEXT_TRY: {
      VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                         ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
      vm_context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
      break;
    }
    case VM_CONTEXT_CATCH: {
      QKING_ASSERT(ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION >
                   ECMA_PARSER_WITH_CONTEXT_STACK_ALLOCATION);

      const uint16_t size_diff = ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION -
                                 ECMA_PARSER_WITH_CONTEXT_STACK_ALLOCATION;

      VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth, size_diff);
      vm_context_top_p -= size_diff;
      ecma_object_t *lex_env_p = frame_ctx_p->lex_env_p;
      frame_ctx_p->lex_env_p = ecma_get_lex_env_outer_reference(lex_env_p);
      ecma_deref_object(lex_env_p);

      VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                         ECMA_PARSER_WITH_CONTEXT_STACK_ALLOCATION);
      vm_context_top_p -= ECMA_PARSER_WITH_CONTEXT_STACK_ALLOCATION;
      break;
    }
    default: {
      QKING_UNREACHABLE();
    }
  }

  return vm_context_top_p;
} /* vm_stack_context_abort */

/**
 * Find a finally up to the end position.
 *
 * @return true - if 'finally' found,
 *         false - otherwise
 */
bool vm_stack_find_finally(
    vm_frame_ctx_t *frame_ctx_p,          /**< frame context */
    ecma_value_t **vm_context_top_ref_p,  /**< current stack top */
    vm_stack_context_type_t finally_type, /**< searching this finally */
    uint32_t search_limit)                /**< search up-to this byte code */
{
  ecma_value_t *vm_context_top_p = *vm_context_top_ref_p;

  QKING_ASSERT(finally_type <= VM_CONTEXT_FINALLY_RETURN);

  if (finally_type != VM_CONTEXT_FINALLY_JUMP) {
    search_limit = 0xffffffffu;
  }

  while (frame_ctx_p->context_depth > 0) {
    vm_stack_context_type_t context_type;
    uint32_t context_end =
        VM_GET_CONTEXT_END(vm_context_top_p[-1]);  //此处已经是绝对偏移了.

    if (search_limit < context_end) {
      *vm_context_top_ref_p = vm_context_top_p;
      return false;
    }

    context_type = VM_GET_CONTEXT_TYPE(vm_context_top_p[-1]);
    if (context_type == VM_CONTEXT_TRY || context_type == VM_CONTEXT_CATCH) {
      unsigned long *byte_code_p;
      uint32_t branch_offset;

      if (search_limit == context_end) {
        *vm_context_top_ref_p = vm_context_top_p;
        return false;
      }

      byte_code_p = frame_ctx_p->pc_start_p + context_end;

      if (context_type == VM_CONTEXT_TRY) {
        if (GET_OP_CODE(byte_code_p[0]) == OP_CATCH) {
          branch_offset = (uint32_t)(GET_ARG_Bx(byte_code_p[0]));

          if (finally_type == VM_CONTEXT_FINALLY_THROW) {
            branch_offset += (uint32_t)(byte_code_p - frame_ctx_p->pc_start_p);

            vm_context_top_p[-1] =
                VM_CREATE_CONTEXT(VM_CONTEXT_CATCH, branch_offset);

            byte_code_p += 1;  // move to next op.
            frame_ctx_p->pc_current_p = byte_code_p;

            *vm_context_top_ref_p = vm_context_top_p;
            return true;
          }

          byte_code_p += branch_offset;

          if (GET_OP_CODE(*byte_code_p) == OP_POP_CONTEXT) {
            VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                               ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
            vm_context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
            continue;
          }
        }
      } else {
        ecma_object_t *lex_env_p = frame_ctx_p->lex_env_p;
        frame_ctx_p->lex_env_p = ecma_get_lex_env_outer_reference(lex_env_p);
        ecma_deref_object(lex_env_p);

        if (GET_OP_CODE(byte_code_p[0]) == OP_POP_CONTEXT) {
          VM_MINUS_EQUAL_U16(frame_ctx_p->context_depth,
                             ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION);
          vm_context_top_p -= ECMA_PARSER_TRY_CONTEXT_STACK_ALLOCATION;
          continue;
        }
      }

      QKING_ASSERT(GET_OP_CODE(byte_code_p[0]) == OP_FINALLY);

      branch_offset = (uint32_t)(GET_ARG_Ax(byte_code_p[0]));

      branch_offset += (uint32_t)(byte_code_p - frame_ctx_p->pc_start_p);

      vm_context_top_p[-1] =
          VM_CREATE_CONTEXT((uint32_t)finally_type, branch_offset);

      byte_code_p += 1;  // move to next op
      frame_ctx_p->pc_current_p = byte_code_p;

      *vm_context_top_ref_p = vm_context_top_p;
      return true;
    }

    vm_context_top_p = vm_stack_context_abort(frame_ctx_p, vm_context_top_p);
  }

  *vm_context_top_ref_p = vm_context_top_p;
  return false;
} /* vm_stack_find_finally */

/**
 * @}
 * @}
 */
