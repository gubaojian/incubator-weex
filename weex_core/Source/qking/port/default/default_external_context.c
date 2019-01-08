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
#include "qking_port.h"
#include "qking_port_default.h"

/**
 * Pointer to the current context.
 * Note that it is a global variable, and is not a thread safe implementation.
 */
static qking_context_t *current_context_p = NULL;

/**
 * Set the current_context_p as the passed pointer.
 */
void
qking_port_default_set_current_context (qking_context_t *context_p) /**< points to the created context */
{
  current_context_p = context_p;
} /* qking_port_default_set_current_context */

/**
 * Get the current context.
 *
 * @return the pointer to the current context
 */
qking_context_t *
qking_port_get_current_context (void)
{
  return current_context_p;
} /* qking_port_get_current_context */
