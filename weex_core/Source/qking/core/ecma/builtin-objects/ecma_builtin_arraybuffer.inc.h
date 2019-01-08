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
 * ArrayBuffer built-in description
 */

#include "ecma_builtin_helpers_macro_defines.inc.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

NUMBER_VALUE(LIT_MAGIC_STRING_LENGTH, 1, ECMA_PROPERTY_FIXED)

/* Object properties:
 *  (property name, object pointer getter) */

OBJECT_VALUE(LIT_MAGIC_STRING_PROTOTYPE, ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE,
             ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the
 * routine's length property) */

/* ES2015 24.1.3.1 */
ROUTINE(LIT_MAGIC_STRING_IS_VIEW_UL, ecma_builtin_arraybuffer_object_is_view, 1,
        1)

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

#include "ecma_builtin_helpers_macro_undefs.inc.h"
