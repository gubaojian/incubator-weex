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

/**
 * Implementation of exit with specified status code.
 */

#include "base/common_logger.h"
#include "jrt.h"
#include "jrt_libc_includes.h"

/*
 * Exit with specified status code.
 *
 * If !QKING_NDEBUG and code != 0, print status code with description
 * and call assertion fail handler.
 */
void QKING_ATTR_NORETURN
qking_fatal (qking_fatal_code_t code) /**< status code */
{
#ifndef QKING_NDEBUG
  switch (code)
  {
    case ERR_OUT_OF_MEMORY:
    {
      QKING_ERROR_MSG ("Error: ERR_OUT_OF_MEMORY\n");
      break;
    }
    case ERR_SYSCALL:
    {
      /* print nothing as it may invoke syscall recursively */
      break;
    }
    case ERR_REF_COUNT_LIMIT:
    {
      QKING_ERROR_MSG ("Error: ERR_REF_COUNT_LIMIT\n");
      break;
    }
    case ERR_DISABLED_BYTE_CODE:
    {
      QKING_ERROR_MSG ("Error: ERR_DISABLED_BYTE_CODE\n");
      break;
    }
    case ERR_FAILED_INTERNAL_ASSERTION:
    {
      QKING_ERROR_MSG ("Error: ERR_FAILED_INTERNAL_ASSERTION\n");
      break;
    }
  }
#endif /* !QKING_NDEBUG */

  qking_port_fatal (code);

  /* to make compiler happy for some RTOS: 'control reaches end of non-void function' */
  while (true)
  {
  }
} /* qking_fatal */

#ifndef QKING_NDEBUG
/**
 * Handle failed assertion
 */
void QKING_ATTR_NORETURN
qking_assert_fail (const char *assertion, /**< assertion condition string */
                   const char *file, /**< file name */
                   const char *function, /**< function name */
                   const uint32_t line) /**< line */
{
  LOGE ("ICE: Assertion '%s' failed at %s(%s):%lu.\n",
                   assertion,
                   file,
                   function,
                   (unsigned long) line);

  qking_fatal (ERR_FAILED_INTERNAL_ASSERTION);
} /* qking_assert_fail */

/**
 * Handle execution of control path that should be unreachable
 */
void QKING_ATTR_NORETURN
qking_unreachable (const char *file, /**< file name */
                   const char *function, /**< function name */
                   const uint32_t line) /**< line */
{
  LOGE ("ICE: Unreachable control path at %s(%s):%lu was executed.\n",
                   file,
                   function,
                   (unsigned long) line);

  qking_fatal (ERR_FAILED_INTERNAL_ASSERTION);
} /* qking_unreachable */
#endif /* !QKING_NDEBUG */
