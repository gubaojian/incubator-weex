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

#include "debugger.h"
#include "jcontext.h"
#include "qking_internal.h"

#ifdef QKING_DEBUGGER

/**
 * Minimum number of bytes transmitted or received.
 */
#define QKING_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE 64

/**
 * Sleep time in milliseconds between each qking_debugger_receive call
 */
#define QKING_DEBUGGER_TRANSPORT_TIMEOUT 100

/**
 * Add a new transport layer.
 */
void
qking_debugger_transport_add (qking_debugger_transport_header_t *header_p, /**< transport implementation */
                              size_t send_message_header_size, /**< header bytes reserved for outgoing messages */
                              size_t max_send_message_size, /**< maximum number of bytes transmitted in a message */
                              size_t receive_message_header_size, /**< header bytes reserved for incoming messages */
                              size_t max_receive_message_size) /**< maximum number of bytes received in a message */
{
  QKING_ASSERT (max_send_message_size > QKING_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE
                && max_receive_message_size > QKING_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE);

  header_p->next_p = QKING_CONTEXT (debugger_transport_header_p);
  QKING_CONTEXT (debugger_transport_header_p) = header_p;

  uint8_t *payload_p;
  size_t max_send_size;
  size_t max_receive_size;

  if (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED)
  {
    payload_p = QKING_CONTEXT (debugger_send_buffer_payload_p);
    max_send_size = QKING_CONTEXT (debugger_max_send_size);
    max_receive_size = QKING_CONTEXT (debugger_max_receive_size);
  }
  else
  {
    QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_CONNECTED);
    payload_p = QKING_CONTEXT (debugger_send_buffer);
    max_send_size = QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE;
    max_receive_size = QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE;
  }

  QKING_ASSERT (max_send_size > QKING_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE + send_message_header_size);
  QKING_ASSERT (max_receive_size > QKING_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE + receive_message_header_size);

  QKING_CONTEXT (debugger_send_buffer_payload_p) = payload_p + send_message_header_size;

  max_send_size = max_send_size - send_message_header_size;
  max_receive_size = max_receive_size - receive_message_header_size;

  if (max_send_size > max_send_message_size)
  {
    max_send_size = max_send_message_size;
  }

  if (max_receive_size > max_receive_message_size)
  {
    max_receive_size = max_receive_message_size;
  }

  QKING_CONTEXT (debugger_max_send_size) = (uint8_t) max_send_size;
  QKING_CONTEXT (debugger_max_receive_size) = (uint8_t) max_receive_size;
} /* qking_debugger_transport_add */

/**
 * Starts the communication to the debugger client.
 * Must be called after the connection is successfully established.
 */
void
qking_debugger_transport_start (void)
{
  QKING_ASSERT (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED);

  if (qking_debugger_send_configuration (QKING_CONTEXT (debugger_max_receive_size)))
  {
    QKING_DEBUGGER_SET_FLAGS (QKING_DEBUGGER_VM_STOP);
    QKING_CONTEXT (debugger_stop_context) = NULL;
  }
} /* qking_debugger_transport_start */

/**
 * Returns true if a debugger client is connected.
 *
 * @return true - a debugger client is connected,
 *         false - otherwise
 */
bool
qking_debugger_transport_is_connected (void)
{
  return (QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED) != 0;
} /* qking_debugger_transport_is_connected */

/**
 * Notifies the debugger server that the connection is closed.
 */
void
qking_debugger_transport_close (void)
{
  if (!(QKING_CONTEXT (debugger_flags) & QKING_DEBUGGER_CONNECTED))
  {
    return;
  }

  QKING_CONTEXT (debugger_flags) = QKING_DEBUGGER_VM_IGNORE;

  qking_debugger_transport_header_t *current_p = QKING_CONTEXT (debugger_transport_header_p);

  QKING_ASSERT (current_p != NULL);

  do
  {
    qking_debugger_transport_header_t *next_p = current_p->next_p;

    current_p->close (current_p);

    current_p = next_p;
  }
  while (current_p != NULL);

  qking_port_log (QKING_LOG_LEVEL_DEBUG, "Debugger client connection closed.\n");

  qking_debugger_free_unreferenced_byte_code ();
} /* qking_debugger_transport_close */

/**
 * Send data over the current connection
 *
 * @return true - data sent successfully,
 *         false - connection closed
 */
bool
qking_debugger_transport_send (const uint8_t *message_p, /**< message to be sent */
                               size_t message_length) /**< message length in bytes */
{
  QKING_ASSERT (qking_debugger_transport_is_connected ());
  QKING_ASSERT (message_length > 0);

  qking_debugger_transport_header_t *header_p = QKING_CONTEXT (debugger_transport_header_p);
  uint8_t *payload_p = QKING_CONTEXT (debugger_send_buffer_payload_p);
  size_t max_send_size = QKING_CONTEXT (debugger_max_send_size);

  do
  {
    size_t fragment_length = (message_length <= max_send_size ? message_length
                                                              : max_send_size);

    memcpy (payload_p, message_p, fragment_length);

    if (!header_p->send (header_p, payload_p, fragment_length))
    {
      return false;
    }

    message_p += fragment_length;
    message_length -= fragment_length;
  }
  while (message_length > 0);

  return true;
} /* qking_debugger_transport_send */

/**
 * Receive data from the current connection
 *
 * Note:
 *   A message is received if message_start_p is not NULL
 *
 * @return true - function successfully completed,
 *         false - connection closed
 */
bool
qking_debugger_transport_receive (qking_debugger_transport_receive_context_t *context_p) /**< [out] receive
                                                                                          *   context */
{
  QKING_ASSERT (qking_debugger_transport_is_connected ());

  context_p->buffer_p = QKING_CONTEXT (debugger_receive_buffer);
  context_p->received_length = QKING_CONTEXT (debugger_received_length);
  context_p->message_p = NULL;
  context_p->message_length = 0;
  context_p->message_total_length = 0;

  qking_debugger_transport_header_t *header_p = QKING_CONTEXT (debugger_transport_header_p);

  return header_p->receive (header_p, context_p);
} /* qking_debugger_transport_receive */

/**
 * Clear the message buffer after the message is processed
 */
void
qking_debugger_transport_receive_completed (qking_debugger_transport_receive_context_t *context_p) /**< receive
                                                                                                    *   context */
{
  QKING_ASSERT (context_p->message_p != NULL);
  QKING_ASSERT (context_p->buffer_p == QKING_CONTEXT (debugger_receive_buffer));

  size_t message_total_length = context_p->message_total_length;
  size_t received_length = context_p->received_length;

  QKING_ASSERT (message_total_length <= received_length);

  if (message_total_length == 0 || message_total_length == received_length)
  {
    /* All received data is processed. */
    QKING_CONTEXT (debugger_received_length) = 0;
    return;
  }

  uint8_t *buffer_p = context_p->buffer_p;
  received_length -= message_total_length;

  memmove (buffer_p, buffer_p + message_total_length, received_length);

  QKING_CONTEXT (debugger_received_length) = (uint16_t) received_length;
} /* qking_debugger_transport_receive_completed */

/**
 * Suspend execution for a predefined time (QKING_DEBUGGER_TRANSPORT_TIMEOUT ms).
 */
void
qking_debugger_transport_sleep (void)
{
  qking_port_sleep (QKING_DEBUGGER_TRANSPORT_TIMEOUT);
} /* qking_debugger_transport_sleep */

#endif /* QKING_DEBUGGER */
