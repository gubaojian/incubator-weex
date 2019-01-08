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
 * RegExp built-in description
 */

#include "ecma_builtin_helpers_macro_defines.inc.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

/* ECMA-262 v5, 15.10.5 */
NUMBER_VALUE(LIT_MAGIC_STRING_LENGTH, 2, ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.10.5.1 */
OBJECT_VALUE(LIT_MAGIC_STRING_PROTOTYPE, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE,
             ECMA_PROPERTY_FIXED)

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

#include "ecma_builtin_helpers_macro_undefs.inc.h"
