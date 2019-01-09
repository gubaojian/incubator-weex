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
/*
 * Boolean.prototype description
 */

#include "ecma_builtin_helpers_macro_defines.inc.h"

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.6.4.1 */
OBJECT_VALUE(LIT_MAGIC_STRING_CONSTRUCTOR, ECMA_BUILTIN_ID_BOOLEAN,
             ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the
 * routine's length property) */
ROUTINE(LIT_MAGIC_STRING_TO_STRING_UL,
        ecma_builtin_boolean_prototype_object_to_string, 0, 0)
ROUTINE(LIT_MAGIC_STRING_VALUE_OF_UL,
        ecma_builtin_boolean_prototype_object_value_of, 0, 0)

#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */

#include "ecma_builtin_helpers_macro_undefs.inc.h"