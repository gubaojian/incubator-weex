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
#ifndef QKINGX_HANDLER_H
#define QKINGX_HANDLER_H

#include "core/include/qking_internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Handler registration helper
 */

qking_value_t qkingx_handler_register_global(
    const qking_char_t *name_p, qking_external_handler_t handler_p);

qking_value_t qkingx_variable_register_global(
    const qking_char_t *name_p, /**< name of the variable */
    qking_value_t var);         /**< variable */
/*
 * Common external function handlers
 */

#ifndef QKING_NDEBUG

qking_value_t qkingx_handler_assert_fatal(const qking_value_t func_obj_val,
                                          const qking_value_t this_p,
                                          const qking_value_t args_p[],
                                          const qking_length_t args_cnt);
qking_value_t qkingx_handler_assert_throw(const qking_value_t func_obj_val,
                                          const qking_value_t this_p,
                                          const qking_value_t args_p[],
                                          const qking_length_t args_cnt);
qking_value_t qkingx_handler_assert(const qking_value_t func_obj_val,
                                    const qking_value_t this_p,
                                    const qking_value_t args_p[],
                                    const qking_length_t args_cnt);

qking_value_t qkingx_handler_create_err(const qking_value_t func_obj_val,
                                        const qking_value_t this_p,
                                        const qking_value_t args_p[],
                                        const qking_length_t args_cnt);

qking_value_t qkingx_handler_gc(const qking_value_t func_obj_val,
                                const qking_value_t this_p,
                                const qking_value_t args_p[],
                                const qking_length_t args_cnt);
qking_value_t qkingx_handler_print(const qking_value_t func_obj_val,
                                   const qking_value_t this_p,
                                   const qking_value_t args_p[],
                                   const qking_length_t args_cnt);

/*
 * Port API extension
 */

/**
 * Print a single character.
 *
 * @param c the character to print.
 */
void qkingx_port_handler_print_char(char c);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKINGX_HANDLER_H */
