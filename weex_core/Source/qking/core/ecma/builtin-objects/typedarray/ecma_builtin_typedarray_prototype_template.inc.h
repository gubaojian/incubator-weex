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
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

#ifndef TYPEDARRAY_BYTES_PER_ELEMENT
#error "Please define TYPEDARRAY_BYTES_PER_ELEMENT"
#endif /* !TYPEDARRAY_BYTES_PER_ELEMENT */

#ifndef TYPEDARRAY_BUILTIN_ID
#error "Please define TYPEDARRAY_BUILTIN_ID"
#endif /* !TYPEDARRAY_BUILTIN_ID */

#include "ecma_builtin_helpers_macro_defines.inc.h"

/* ES2015 22.2.3.4 */
OBJECT_VALUE(LIT_MAGIC_STRING_CONSTRUCTOR, TYPEDARRAY_BUILTIN_ID,
             ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* ES2015 22.2.6.1 */
NUMBER_VALUE(LIT_MAGIC_STRING_BYTES_PER_ELEMENT_U, TYPEDARRAY_BYTES_PER_ELEMENT,
             ECMA_PROPERTY_FIXED)

#include "ecma_builtin_helpers_macro_undefs.inc.h"

#undef TYPEDARRAY_BUILTIN_ID
#undef TYPEDARRAY_BYTES_PER_ELEMENT

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */