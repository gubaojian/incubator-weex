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
#ifndef QKING_PORT_DEFAULT_H
#define QKING_PORT_DEFAULT_H

#include <stdbool.h>

#include "qking_internal.h"
#include "qking_port.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \addtogroup qking_port_default Default Qking engine port API
 * These functions are only available if the default port of Qking is used.
 * @{
 */

void qking_port_default_set_abort_on_fail(bool flag);
bool qking_port_default_is_abort_on_fail(void);

qking_log_level_t qking_port_default_get_log_level(void);
void qking_port_default_set_log_level(qking_log_level_t level);

void qking_port_default_set_current_context(qking_context_t *context_p);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !QKING_PORT_DEFAULT_H */
