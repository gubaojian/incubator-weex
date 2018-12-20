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

#include "base/LogDefines.h"
#include "vnode_qking_convert.h"

namespace weex {
namespace core {
namespace data_render {

struct travel_qking_struct {
  bool same;
  qking_value_t right;
};


static bool for_each_equal_check(const qking_value_t property_name, const qking_value_t property_value,void *user_data_p){
    travel_qking_struct* result_p = (travel_qking_struct *)user_data_p;
    const qking_value_t right = result_p->right;
    qking_value_t prop_in_right = qking_get_property(right, property_name);
    if (qking_value_is_error(prop_in_right)){
        //not found
        result_p->same = false;
        return false;
    }

    //found
    if(!qking_value_deep_compare(property_value,prop_in_right)){
        //not same
        result_p->same = false;
        return false;
    }
    //same, continue;
    return true;
}

static bool for_each_check_count(const qking_value_t property_name, const qking_value_t property_value,void *user_data_p){
    *((size_t *)user_data_p)+=1;
    return true;
}

bool qking_value_deep_compare(const qking_value_t left,const qking_value_t right){
    qking_type_t left_type = qking_value_get_type(left);
    qking_type_t right_type = qking_value_get_type(right);

    if (left_type != right_type){
        return false;
    }

    switch (left_type) {
        case QKING_TYPE_NONE:
        case QKING_TYPE_UNDEFINED:
        case QKING_TYPE_NULL:{
            return true;
        }
        case QKING_TYPE_BOOLEAN: {
            return left == right;
        }
        case QKING_TYPE_NUMBER:
        case QKING_TYPE_STRING:
        case QKING_TYPE_FUNCTION:{
            return qking_value_strict_equal(left,right_type);
        }
        case QKING_TYPE_OBJECT: {
            travel_qking_struct same;

            //check property count;
            size_t left_size;
            size_t right_size;
            qking_foreach_object_property(left,for_each_check_count, &left_size);
            qking_foreach_object_property(right,for_each_check_count, &right_size);
            if (left_size!=right_size){
                return false;
            }

            same.same = true;
            if (!qking_foreach_object_property(left,for_each_equal_check, &same)){
                return false;
            }
            return same.same;
        }
        case QKING_TYPE_ERROR: {
            return false;//wtf?
        }
        default:{
            return false;
        }
    }
    return false;
}

bool qking_value_add_all_to(const qking_value_t src,const qking_value_t dest){
    if (!qking_value_is_object(src) || !qking_value_is_object(dest)) {
        return false;
    }

    qking_foreach_object_property(src, [](const qking_value_t property_name,
                                          const qking_value_t property_value,
                                          void* user_data_p) {
      qking_value_t dest_in = *((qking_value_t*) user_data_p);
      qking_set_property(dest_in, property_name, property_value);
      return true;
    }, (void*) &dest);
    return true;
}

std::string string_from_qking_string_value(const qking_value_t string_var) {
    std::string str;
    if (qking_value_is_string(string_var)) {
        uint32_t string_size = qking_get_string_size(string_var);
        qking_char_t *buffer = new qking_char_t[string_size];
        if (buffer) {
            qking_size_t result_size = qking_string_to_char_buffer(string_var, buffer, string_size);
            std::string str((char *)buffer, result_size);
            delete []buffer;
            return str;
        }
    }
    return str;
}

std::string string_from_qking_error(const qking_value_t err) {
    if (!qking_value_is_error(err)) {
        return "not an err";
    }

    qking_value_t err_value = qking_get_value_from_error(err, false);
    qking_value_t str_val = qking_value_to_string(err_value);
    qking_release_value(err_value);

    if (qking_value_is_error(str_val)){
        qking_release_value(str_val);
        return "err can' convert to string";
    }

    return string_from_qking_string_value(str_val);
}

std::string string_from_qking_get_property_by_name(const qking_value_t obj_val, const char *name_p) {
    qking_value_t result = qking_get_property_by_name(obj_val, name_p);
    std::string str = string_from_qking_string_value(result);
    qking_release_value(result);
    return str;
}

std::string string_from_qking_json_stringify(const qking_value_t object_to_stringify) {
    qking_value_t string_var = qking_json_stringify(object_to_stringify);
    if(qking_value_is_error(string_var)){
        const std::string& err_log = string_from_qking_error(string_var);
        LOGE("[qking] string_from_qking_json_stringify err: %s",err_log.c_str());
        qking_release_value(string_var);
        return "";
    }
    std::string str = string_from_qking_string_value(string_var);
    qking_release_value(string_var);
    return str;
}

std::string string_from_qking_get_property_by_index(const qking_value_t obj_val, uint32_t index) {
    qking_value_t string_var = qking_get_property_by_index(obj_val, index);
    std::string str = string_from_qking_string_value(string_var);
    qking_release_value(string_var);
    return str;
}

bool string_from_qking_execute_code(qking_executor_t executor, std::string &error) {
    qking_value_t error_var = qking_create_undefined();
    bool success = qking_execute_code(executor, &error_var);
    error = string_from_qking_string_value(error_var);
    qking_release_value(error_var);
    return success;
}

bool string_from_qking_set_assembly_code(qking_executor_t executor, uint8_t *code, size_t size, std::string &error) {
    qking_value_t error_var = qking_create_undefined();
    bool success = qking_set_assembly_code(executor, code, size, &error_var);
    if (!success) {
        error = string_from_qking_string_value(error_var);
        qking_release_value(error_var);
    }
    return success;
}

#ifndef CONFIG_DISABLE_COMPILER_BUILTIN

bool string_from_qking_set_compile_code(qking_executor_t executor, const char *pstr, std::string &error)
{
    qking_value_t error_var = qking_create_undefined();
    bool success = qking_set_compile_code(executor, pstr, &error_var);
    if (!success) {
        error = string_from_qking_string_value(error_var);
        qking_release_value(error_var);
    }
    return success;
}

#endif


#ifdef DEBUG
std::string qking_value_print(const qking_value_t value) {
    qking_value_t string = qking_value_debug_print(value);
    std::string str = string_from_qking_string_value(string);
    qking_release_value(string);
    return str;
}
#endif

}  // namespace data_render
}  // namespace core
}  // namespace weex
