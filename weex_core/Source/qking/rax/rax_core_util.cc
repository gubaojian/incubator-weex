//
// Created by Xu Jiacheng on 2018/12/25.
//

#include "rax_core_util.h"

RAX_NAME_SPACE_BEGIN

static void push_all_recursive(qking_value_t src, qking_value_t dest,
                               qking_length_t &index) {
  if (qking_value_is_array(src)) {
    uint32_t array_length = qking_get_array_length(src);
    for (uint32_t i = 0; i < array_length; ++i) {
      qking_value_t a_var = qking_get_property_by_index(src, i);
      if (qking_value_is_error(a_var)) {
        qking_release_value(a_var);
        RAX_LOGE("Error push_all_recursive");
        break;
      }
      push_all_recursive(a_var, dest, index);
      qking_release_value(a_var);
    }
  } else {
    qking_release_value(qking_set_property_by_index(dest, index, src));
    index++;
  }
}

qking_value_t rax_flatten_children(const qking_value_t *array,
                                   qking_length_t size) {
  if (size == 0) {
    return qking_create_undefined();
  }
  // only child not array
  if (size == 1 && !qking_value_is_array(array[0])) {
    return qking_acquire_value(array[0]);
  }

  qking_value_t result_array = qking_create_array(0);
  qking_length_t index = 0;
  for (qking_length_t i = 0; i < size; ++i) {
    qking_value_t it = array[i];
    push_all_recursive(it, result_array, index);
  }
  return result_array;
}

qking_value_t rax_flatten_style(qking_value_t value) {
  if (qking_value_is_null_or_undefined(value)) {
    return qking_create_undefined();
  }

  return qking_acquire_value(value);  // todo support array style later.
}

bool qking_value_add_all_to(const qking_value_t dest, const qking_value_t src) {
  if (!qking_value_is_object(src) || !qking_value_is_object(dest)) {
    return false;
  }

  qking_foreach_object_property_of(
      src,
      [](const qking_value_t property_name, const qking_value_t property_value,
         void *user_data_p) {
        qking_value_t dest_in = *((qking_value_t *)user_data_p);
        qking_set_property(dest_in, property_name, property_value);
        return true;
      },
      (void *)&dest, false, true, false);
  return true;
}

std::string string_from_qking_string_value_lit(const qking_magic_str_t name) {
  std::string str;
  qking_value_t string_var = qking_create_string_lit(name);
  if (qking_value_is_string(string_var)) {
    uint32_t string_size = qking_get_string_size(string_var);
    qking_char_t *buffer = new qking_char_t[string_size];
    if (buffer) {
      qking_size_t result_size =
          qking_string_to_char_buffer(string_var, buffer, string_size);
      std::string str((char *)buffer, result_size);
      delete[] buffer;
      qking_release_value(string_var);
      return str;
    }
  }
  qking_release_value(string_var);
  return str;
}

std::string string_from_qking_string_value(const qking_value_t string_var) {
  std::string str;
  if (qking_value_is_string(string_var)) {
    uint32_t string_size = qking_get_string_size(string_var);
    qking_char_t *buffer = new qking_char_t[string_size];
    if (buffer) {
      qking_size_t result_size =
          qking_string_to_char_buffer(string_var, buffer, string_size);
      std::string str((char *)buffer, result_size);
      delete[] buffer;
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

  if (qking_value_is_error(str_val)) {
    qking_release_value(str_val);
    return "err can' convert to string";
  }

  const std::string &err_msg_str = string_from_qking_string_value(str_val);
  qking_release_value(str_val);
  return err_msg_str;
}

std::string string_from_qking_get_property_by_name(const qking_value_t obj_val,
                                                   const char *name_p) {
  qking_value_t result = qking_get_property_by_name(obj_val, name_p);
  std::string str = string_from_qking_string_value(result);
  qking_release_value(result);
  return str;
}

std::string string_from_qking_json_stringify(
    const qking_value_t object_to_stringify) {
  if (qking_value_is_undefined(object_to_stringify)) {
    return "undefined";
  }

  qking_value_t string_var = qking_json_stringify(object_to_stringify);
  if (qking_value_is_error(string_var)) {
    const std::string &err_log = string_from_qking_error(string_var);
    RAX_LOGE("[qking] string_from_qking_json_stringify err: %s",
             err_log.c_str());
    qking_release_value(string_var);
    return "";
  }
  std::string str = string_from_qking_string_value(string_var);
  qking_release_value(string_var);
  return str;
}

std::string string_from_qking_to_string(const qking_value_t object_to_string) {
  qking_value_t string_var = qking_value_to_string(object_to_string);
  if (qking_value_is_error(string_var)) {
    const std::string &err_log = string_from_qking_error(string_var);
    RAX_LOGE("[qking] string_from_qking_to_string err: %s", err_log.c_str());
    qking_release_value(string_var);
    return "";
  }
  std::string str = string_from_qking_string_value(string_var);
  qking_release_value(string_var);
  return str;
}

std::string string_from_qking_get_property_by_index(const qking_value_t obj_val,
                                                    uint32_t index) {
  qking_value_t string_var = qking_get_property_by_index(obj_val, index);
  std::string str = string_from_qking_string_value(string_var);
  qking_release_value(string_var);
  return str;
}

void make_optional_call_on(qking_value_t class_inst,
                           qking_magic_str_t func_name, const char *process_msg,
                           const qking_value_t *args_p,
                           qking_size_t args_count) {
  // componentWillMount
  qking_value_t func_by_name =
      qking_get_property_by_name_lit(class_inst, func_name);

  if (qking_value_is_error(func_by_name)) {
    RAX_LOGE("%s: Get func %s err: %s", process_msg,
             string_from_qking_string_value_lit(func_name).c_str(),
             string_from_qking_error(func_by_name).c_str());
  } else if (qking_value_is_function(func_by_name)) {
    qking_value_t ret =
        qking_call_function(func_by_name, class_inst, args_p, args_count);
    if (qking_value_is_error(ret)) {
      RAX_LOGE("%s: Call %s err: %s", process_msg,
               string_from_qking_string_value_lit(func_name).c_str(),
               string_from_qking_error(ret).c_str());
    }
    qking_release_value(ret);
  } /* else { <not found> } */

  qking_release_value(func_by_name);
}

qking_value_t make_optional_call_on_ret(qking_value_t class_inst,
                                        qking_magic_str_t func_name,
                                        const char *process_msg,
                                        bool &has_function,
                                        const qking_value_t *args_p,
                                        qking_size_t args_count) {
  // componentWillMount
  qking_value_t ret = qking_create_undefined();
  qking_value_t func_by_name =
      qking_get_property_by_name_lit(class_inst, func_name);

  if (qking_value_is_error(func_by_name)) {
    RAX_LOGE("%s: Get func %s err: %s", process_msg,
             string_from_qking_string_value_lit(func_name).c_str(),
             string_from_qking_error(func_by_name).c_str());
    has_function = false;
  } else if (qking_value_is_function(func_by_name)) {
    ret = qking_call_function(func_by_name, class_inst, args_p, args_count);
    if (qking_value_is_error(ret)) {
      RAX_LOGE("%s: Call %s err: %s", process_msg,
               string_from_qking_string_value_lit(func_name).c_str(),
               string_from_qking_error(ret).c_str());
      qking_release_value(ret);
      ret = qking_create_undefined();
    }
    has_function = true;
  } else {
    has_function = false;
  }

  qking_release_value(func_by_name);
  return ret;
}

qking_value_t get_optional_property(
    qking_value_t obj, qking_magic_str_t prop_name, const char *process_msg,
    const char *obj_name, std::function<void(qking_value_t)> succ_callback) {
  qking_value_t ret = qking_get_property_by_name_lit(obj, prop_name);
  if (qking_value_is_error(ret)) {
    RAX_LOGE("%s: %s['%s'] err: %s", process_msg, obj_name,
             string_from_qking_string_value_lit(prop_name).c_str(),
             string_from_qking_error(ret).c_str());
    qking_release_value(ret);
    ret = qking_create_undefined();
  } else {
    if (succ_callback) {
      succ_callback(ret);
    }
  }
  return ret;
}

bool set_optional_property(qking_value_t obj, qking_magic_str_t prop_name,
                           qking_value_t value, const char *process_msg,
                           const char *obj_msg) {
  bool succ = true;
  qking_value_t ret = qking_set_property_by_name_lit(obj, prop_name, value);
  if (qking_value_is_error(ret)) {
    RAX_LOGE("%s: %s['%s'] set err: %s", process_msg, obj_msg,
             string_from_qking_string_value_lit(prop_name).c_str(),
             string_from_qking_error(ret).c_str());
    succ = false;
  }
  qking_release_value(ret);
  return succ;
}

QKValueRef::QKValueRef(qking_value_t value) {
  value_ = qking_acquire_value(value);
}

QKValueRef::~QKValueRef() { qking_release_value(value_); }

rax_common_err::rax_common_err(const char *err) : logic_error(err) {}
rax_common_err::rax_common_err(const std::string &err) : logic_error(err) {}

RAX_NAME_SPACE_END
