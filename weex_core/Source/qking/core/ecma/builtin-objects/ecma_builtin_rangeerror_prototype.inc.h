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
 * RangeError.prototype built-in description
 */

#include "ecma_builtin_helpers_macro_defines.inc.h"

#ifndef CONFIG_DISABLE_ERROR_BUILTINS

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.11.7.8 */
OBJECT_VALUE(LIT_MAGIC_STRING_CONSTRUCTOR, ECMA_BUILTIN_ID_RANGE_ERROR,
             ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* ECMA-262 v5, 15.11.7.9 */
STRING_VALUE(LIT_MAGIC_STRING_NAME, LIT_MAGIC_STRING_RANGE_ERROR_UL,
             ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* ECMA-262 v5, 15.11.7.10 */
STRING_VALUE(LIT_MAGIC_STRING_MESSAGE, LIT_MAGIC_STRING__EMPTY,
             ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */

#include "ecma_builtin_helpers_macro_undefs.inc.h"
