/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecma_array_object.h"
#include "ecma_helpers.h"
#include "jcontext.h"
#include "vm.h"

/**
 * Check whether currently executed code is strict mode code
 *
 * @return true - current code is executed in strict mode,
 *         false - otherwise
 */
bool vm_is_strict_mode (void)
{
  QKING_ASSERT (QKING_CONTEXT (vm_top_context_p) != NULL);

  return QKING_CONTEXT (vm_top_context_p)->bytecode_header_p->status_flags & ECMA_CODE_FLAGS_STRICT_MODE;
} /* vm_is_strict_mode */
