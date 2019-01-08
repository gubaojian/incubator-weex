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
#include <stdio.h>
#include "handler.h"

#ifdef QKING_DEBUGGER

#define DEBUG_BUFFER_SIZE (256)
static char debug_buffer[DEBUG_BUFFER_SIZE];
static int debug_buffer_index = 0;

#endif /* QKING_DEBUGGER */

/**
 * Default implementation of qkingx_port_handler_print_char. Uses 'printf' to
 * print a single character to standard output.
 */
void
qkingx_port_handler_print_char (char c) /**< the character to print */
{
  printf ("%c", c);

#ifdef QKING_DEBUGGER
  debug_buffer[debug_buffer_index++] = c;

  if ((debug_buffer_index == DEBUG_BUFFER_SIZE) || (c == '\n'))
  {
    qking_debugger_send_output ((qking_char_t *) debug_buffer, (qking_size_t) debug_buffer_index);
    debug_buffer_index = 0;
  }
#endif /* QKING_DEBUGGER */
} /* qkingx_port_handler_print_char */
