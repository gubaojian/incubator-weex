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
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "ecma_globals.h"
#include "qking_debugger_transport.h"

#ifdef QKING_DEBUGGER

/* Qking debugger protocol is a simplified version of RFC-6455
 * (WebSockets). */

/**
 * Frequency of calling qking_debugger_receive() by the VM.
 */
#define QKING_DEBUGGER_MESSAGE_FREQUENCY 5

/**
 * This constant represents that the string to be sent has no subtype.
 */
#define QKING_DEBUGGER_NO_SUBTYPE 0

/**
 * Limited resources available for the engine, so it is important to
 * check the maximum buffer size. It needs to be between 64 and 256 bytes.
 */
#if QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE < 64 || \
    QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE > 256
#error Please define the MAX_BUFFER_SIZE between 64 and 256 bytes.
#endif /* QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE < 64 || \
          QKING_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE > 256 */

/**
 * Calculate the maximum number of items for a given type
 * which can be transmitted in one message.
 */
#define QKING_DEBUGGER_SEND_MAX(type)                \
  ((size_t)((QKING_CONTEXT(debugger_max_send_size) - \
             sizeof(qking_debugger_send_type_t)) /   \
            sizeof(type)))

/**
 * Calculate the size of a message when a count number of items transmitted.
 */
#define QKING_DEBUGGER_SEND_SIZE(count, type) \
  ((size_t)((count * sizeof(type)) + sizeof(qking_debugger_send_type_t)))

/**
 * Debugger operation modes:
 *
 * The debugger has two operation modes: run mode and breakpoint mode.
 *
 * In run mode the debugger server accepts only a limited number of message
 * types from the debugger client (e.g. stop execution, set breakpoint).
 *
 * In breakpoint mode the JavaScript execution is stopped at a breakpoint and
 * more message types are accepted (e.g. get backtrace, evaluate expression).
 *
 * Switching between modes:
 *
 * When the JavaScript execution stops at a breakpoint the server sends a
 * QKING_DEBUGGER_BREAKPOINT_HIT message to the client. The client can only
 * issue breakpoint mode commands after this message is received.
 *
 * Certain breakpoint mode commands (e.g. continue) resumes the JavaScript
 * execution and the client must not send any breakpoint mode messages
 * until the QKING_DEBUGGER_BREAKPOINT_HIT is received again.
 *
 * The debugger server starts in run mode but stops at the first available
 * breakpoint.
 */

/**
 * Debugger option flags.
 */
typedef enum {
  QKING_DEBUGGER_CONNECTED = 1u << 0, /**< debugger is connected */
  QKING_DEBUGGER_BREAKPOINT_MODE =
      1u << 1, /**< debugger waiting at a breakpoint */
  QKING_DEBUGGER_VM_STOP =
      1u << 2, /**< stop at the next breakpoint even if disabled */
  QKING_DEBUGGER_VM_IGNORE = 1u << 3, /**< ignore all breakpoints */
  QKING_DEBUGGER_VM_IGNORE_EXCEPTION =
      1u << 4, /**< debugger doesn't stop at any exception */
  QKING_DEBUGGER_VM_EXCEPTION_THROWN =
      1u << 5, /**< no need to stop for this exception */
  QKING_DEBUGGER_PARSER_WAIT =
      1u << 6, /**< debugger should wait after parsing is completed */
  QKING_DEBUGGER_PARSER_WAIT_MODE =
      1u << 7, /**< debugger is waiting after parsing is completed */
  QKING_DEBUGGER_CLIENT_SOURCE_MODE =
      1u << 8, /**< debugger waiting for client code */
  QKING_DEBUGGER_CLIENT_NO_SOURCE =
      1u << 9, /**< debugger leaving the client source loop */
  QKING_DEBUGGER_CONTEXT_RESET_MODE =
      1u << 10, /**< debugger and engine reinitialization mode */
} qking_debugger_flags_t;

/**
 * Set debugger flags.
 */
#define QKING_DEBUGGER_SET_FLAGS(flags) \
  QKING_CONTEXT(debugger_flags) =       \
      (QKING_CONTEXT(debugger_flags) | (uint32_t)(flags))

/**
 * Clear debugger flags.
 */
#define QKING_DEBUGGER_CLEAR_FLAGS(flags) \
  QKING_CONTEXT(debugger_flags) =         \
      (QKING_CONTEXT(debugger_flags) & (uint32_t) ~(flags))

/**
 * Set and clear debugger flags.
 */
#define QKING_DEBUGGER_UPDATE_FLAGS(flags_to_set, flags_to_clear)   \
  QKING_CONTEXT(debugger_flags) =                                   \
      ((QKING_CONTEXT(debugger_flags) | (uint32_t)(flags_to_set)) & \
       (uint32_t) ~(flags_to_clear))

/**
 * Types for the package.
 */
typedef enum {
  /* Messages sent by the server to client. */
  /* This is a handshake message, sent once during initialization. */
  QKING_DEBUGGER_CONFIGURATION = 1, /**< debugger configuration */
  /* These messages are sent by the parser. */
  QKING_DEBUGGER_PARSE_ERROR = 2,     /**< parse error */
  QKING_DEBUGGER_BYTE_CODE_CP = 3,    /**< byte code compressed pointer */
  QKING_DEBUGGER_PARSE_FUNCTION = 4,  /**< parsing a new function */
  QKING_DEBUGGER_BREAKPOINT_LIST = 5, /**< list of line offsets */
  QKING_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6, /**< list of byte code offsets */
  QKING_DEBUGGER_SOURCE_CODE = 7,            /**< source code fragment */
  QKING_DEBUGGER_SOURCE_CODE_END = 8,        /**< source code last fragment */
  QKING_DEBUGGER_SOURCE_CODE_NAME = 9,       /**< source code name fragment */
  QKING_DEBUGGER_SOURCE_CODE_NAME_END =
      10,                                /**< source code name last fragment */
  QKING_DEBUGGER_FUNCTION_NAME = 11,     /**< function name fragment */
  QKING_DEBUGGER_FUNCTION_NAME_END = 12, /**< function name last fragment */
  QKING_DEBUGGER_WAITING_AFTER_PARSE =
      13, /**< engine waiting for a parser resume */
  /* These messages are generic messages. */
  QKING_DEBUGGER_RELEASE_BYTE_CODE_CP =
      14, /**< invalidate byte code compressed pointer */
  QKING_DEBUGGER_MEMSTATS_RECEIVE = 15,  /**< memstats sent to the client */
  QKING_DEBUGGER_BREAKPOINT_HIT = 16,    /**< notify breakpoint hit */
  QKING_DEBUGGER_EXCEPTION_HIT = 17,     /**< notify exception hit */
  QKING_DEBUGGER_EXCEPTION_STR = 18,     /**< exception string fragment */
  QKING_DEBUGGER_EXCEPTION_STR_END = 19, /**< exception string last fragment */
  QKING_DEBUGGER_BACKTRACE_TOTAL = 20,   /**< number of total frames */
  QKING_DEBUGGER_BACKTRACE = 21,         /**< backtrace data */
  QKING_DEBUGGER_BACKTRACE_END = 22,     /**< last backtrace data */
  QKING_DEBUGGER_EVAL_RESULT = 23,       /**< eval result */
  QKING_DEBUGGER_EVAL_RESULT_END = 24,   /**< last part of eval result */
  QKING_DEBUGGER_WAIT_FOR_SOURCE = 25,   /**< engine waiting for source code */
  QKING_DEBUGGER_OUTPUT_RESULT =
      26, /**< output sent by the program to the debugger */
  QKING_DEBUGGER_OUTPUT_RESULT_END = 27, /**< last output result data */
  QKING_DEBUGGER_SCOPE_CHAIN = 28,       /**< scope chain */
  QKING_DEBUGGER_SCOPE_CHAIN_END = 29,   /**< last output of scope chain */
  QKING_DEBUGGER_SCOPE_VARIABLES = 30,   /**< scope variables */
  QKING_DEBUGGER_SCOPE_VARIABLES_END =
      31,                                /**< last output of scope variables */
  QKING_DEBUGGER_MESSAGES_OUT_MAX_COUNT, /**< number of different type of output
                                            messages by the debugger */

  /* Messages sent by the client to server. */

  /* The following messages are accepted in both run and breakpoint modes. */
  QKING_DEBUGGER_FREE_BYTE_CODE_CP =
      1, /**< free byte code compressed pointer */
  QKING_DEBUGGER_UPDATE_BREAKPOINT = 2, /**< update breakpoint status */
  QKING_DEBUGGER_EXCEPTION_CONFIG = 3,  /**< exception handler config */
  QKING_DEBUGGER_PARSER_CONFIG = 4,     /**< parser config */
  QKING_DEBUGGER_MEMSTATS = 5,          /**< list memory statistics */
  QKING_DEBUGGER_STOP = 6,              /**< stop execution */
  /* The following message is only available in waiting after parse mode. */
  QKING_DEBUGGER_PARSER_RESUME = 7, /**< stop waiting after parse */
  /* The following four messages are only available in client switch mode. */
  QKING_DEBUGGER_CLIENT_SOURCE = 8,      /**< first message of client source */
  QKING_DEBUGGER_CLIENT_SOURCE_PART = 9, /**< next message of client source */
  QKING_DEBUGGER_NO_MORE_SOURCES = 10,   /**< no more sources notification */
  QKING_DEBUGGER_CONTEXT_RESET = 11,     /**< context reset request */
  /* The following messages are only available in breakpoint
   * mode and they switch the engine to run mode. */
  QKING_DEBUGGER_CONTINUE = 12, /**< continue execution */
  QKING_DEBUGGER_STEP = 13,     /**< next breakpoint, step into functions */
  QKING_DEBUGGER_NEXT = 14,     /**< next breakpoint in the same context */
  QKING_DEBUGGER_FINISH = 15,   /**< Continue running just after the function in
                                   the current stack frame returns */
  /* The following messages are only available in breakpoint
   * mode and this mode is kept after the message is processed. */
  QKING_DEBUGGER_GET_BACKTRACE = 16, /**< get backtrace */
  QKING_DEBUGGER_EVAL = 17,      /**< first message of evaluating a string */
  QKING_DEBUGGER_EVAL_PART = 18, /**< next message of evaluating a string */
  QKING_DEBUGGER_GET_SCOPE_CHAIN = 19, /**< get type names of the scope chain */
  QKING_DEBUGGER_GET_SCOPE_VARIABLES = 20, /**< get variables of a scope */
  QKING_DEBUGGER_MESSAGES_IN_MAX_COUNT, /**< number of different type of input
                                           messages */
} qking_debugger_header_type_t;

/**
 * Debugger option flags.
 */
typedef enum {
  QKING_DEBUGGER_LITTLE_ENDIAN = 1u << 0, /**< little endian */
} qking_debugger_configuration_flags_t;

/**
 * Subtypes of eval.
 */
typedef enum {
  QKING_DEBUGGER_EVAL_EVAL = 0, /**< evaluate expression */
  QKING_DEBUGGER_EVAL_THROW =
      1, /**< evaluate expression and throw the result */
  QKING_DEBUGGER_EVAL_ABORT =
      2, /**< evaluate expression and abrot with the result */
} qking_debugger_eval_type_t;

/**
 * Subtypes of eval_result.
 */
typedef enum {
  QKING_DEBUGGER_EVAL_OK = 1,    /**< eval result, no error */
  QKING_DEBUGGER_EVAL_ERROR = 2, /**< eval result when an error has occurred */
} qking_debugger_eval_result_type_t;

/**
 * Subtypes of output_result.
 *
 * Note:
 *      This enum has to be kept in sync with qking_log_level_t with an offset
 *      of +2.
 */
typedef enum {
  QKING_DEBUGGER_OUTPUT_OK = 1,      /**< output result, no error */
  QKING_DEBUGGER_OUTPUT_ERROR = 2,   /**< output result, error */
  QKING_DEBUGGER_OUTPUT_WARNING = 3, /**< output result, warning */
  QKING_DEBUGGER_OUTPUT_DEBUG = 4,   /**< output result, debug */
  QKING_DEBUGGER_OUTPUT_TRACE = 5,   /**< output result, trace */
} qking_debugger_output_subtype_t;

/**
 * Types of scopes.
 */
typedef enum {
  QKING_DEBUGGER_SCOPE_WITH = 1,       /**< with */
  QKING_DEBUGGER_SCOPE_LOCAL = 2,      /**< local */
  QKING_DEBUGGER_SCOPE_CLOSURE = 3,    /**< closure */
  QKING_DEBUGGER_SCOPE_GLOBAL = 4,     /**< global */
  QKING_DEBUGGER_SCOPE_NON_CLOSURE = 5 /**< non closure */
} qking_debugger_scope_chain_type_t;

/**
 * Type of scope variables.
 */
typedef enum {
  QKING_DEBUGGER_VALUE_NONE = 1,
  QKING_DEBUGGER_VALUE_UNDEFINED = 2,
  QKING_DEBUGGER_VALUE_NULL = 3,
  QKING_DEBUGGER_VALUE_BOOLEAN = 4,
  QKING_DEBUGGER_VALUE_NUMBER = 5,
  QKING_DEBUGGER_VALUE_STRING = 6,
  QKING_DEBUGGER_VALUE_FUNCTION = 7,
  QKING_DEBUGGER_VALUE_ARRAY = 8,
  QKING_DEBUGGER_VALUE_OBJECT = 9
} qking_debugger_scope_variable_type_t;

/**
 * Byte data for evaluating expressions and receiving client source.
 */
typedef struct {
  uint32_t uint8_size;   /**< total size of the client source */
  uint32_t uint8_offset; /**< current offset in the client source */
} qking_debugger_uint8_data_t;

/**
 * Delayed free of byte code data.
 */
typedef struct {
  uint16_t size; /**< size of the byte code header divided by JMEM_ALIGNMENT */
  jmem_cpointer_t prev_cp; /**< previous byte code data to be freed */
} qking_debugger_byte_code_free_t;

/**
 * Outgoing message: Qking configuration.
 */
typedef struct {
  uint8_t type;                      /**< type of the message */
  uint8_t configuration;             /**< configuration option bits */
  uint8_t version[sizeof(uint32_t)]; /**< debugger version */
  uint8_t max_message_size;          /**< maximum incoming message size */
  uint8_t cpointer_size;             /**< size of compressed pointers */
} qking_debugger_send_configuration_t;

/**
 * Outgoing message: message without arguments.
 */
typedef struct {
  uint8_t type; /**< type of the message */
} qking_debugger_send_type_t;

/**
 * Incoming message: message without arguments.
 */
typedef struct {
  uint8_t type; /**< type of the message */
} qking_debugger_receive_type_t;

/**
 * Outgoing message: string (Source file name or function name).
 */
typedef struct {
  uint8_t type;     /**< type of the message */
  uint8_t string[]; /**< string data */
} qking_debugger_send_string_t;

/**
 * Outgoing message: uint32 value.
 */
typedef struct {
  uint8_t type;                     /**< type of the message */
  uint8_t line[sizeof(uint32_t)];   /**< value data */
  uint8_t column[sizeof(uint32_t)]; /**< value data */
} qking_debugger_send_parse_function_t;

/**
 * Outgoing message: byte code compressed pointer.
 */
typedef struct {
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof(
      jmem_cpointer_t)]; /**< byte code compressed pointer */
} qking_debugger_send_byte_code_cp_t;

/**
 * Incoming message: byte code compressed pointer.
 */
typedef struct {
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof(
      jmem_cpointer_t)]; /**< byte code compressed pointer */
} qking_debugger_receive_byte_code_cp_t;

/**
 * Incoming message: update (enable/disable) breakpoint status.
 */
typedef struct {
  uint8_t type;              /**< type of the message */
  uint8_t is_set_breakpoint; /**< set or clear breakpoint */
  uint8_t byte_code_cp[sizeof(
      jmem_cpointer_t)];            /**< byte code compressed pointer */
  uint8_t offset[sizeof(uint32_t)]; /**< breakpoint offset */
} qking_debugger_receive_update_breakpoint_t;

/**
 * Outgoing message: send memory statistics
 */
typedef struct {
  uint8_t type;                              /**< type of the message */
  uint8_t allocated_bytes[sizeof(uint32_t)]; /**< allocated bytes */
  uint8_t byte_code_bytes[sizeof(uint32_t)]; /**< byte code bytes */
  uint8_t string_bytes[sizeof(uint32_t)];    /**< string bytes */
  uint8_t object_bytes[sizeof(uint32_t)];    /**< object bytes */
  uint8_t property_bytes[sizeof(uint32_t)];  /**< property bytes */
} qking_debugger_send_memstats_t;

/**
 * Outgoing message: notify breakpoint hit.
 */
typedef struct {
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof(
      jmem_cpointer_t)];            /**< byte code compressed pointer */
  uint8_t offset[sizeof(uint32_t)]; /**< breakpoint offset */
} qking_debugger_send_breakpoint_hit_t;

/**
 * Stack frame descriptor for sending backtrace information.
 */
typedef struct {
  uint8_t byte_code_cp[sizeof(
      jmem_cpointer_t)];            /**< byte code compressed pointer */
  uint8_t offset[sizeof(uint32_t)]; /**< last breakpoint offset */
} qking_debugger_frame_t;

/**
 * Outgoing message: backtrace information.
 */
typedef struct {
  uint8_t type;                    /**< type of the message */
  qking_debugger_frame_t frames[]; /**< frames */
} qking_debugger_send_backtrace_t;

/**
 * Outgoing message: scope chain.
 */
typedef struct {
  uint8_t type;          /**< type of the message */
  uint8_t scope_types[]; /**< scope types */
} qking_debugger_send_scope_chain_t;

/**
 * Outgoing message: number of total frames in backtrace.
 */
typedef struct {
  uint8_t type;                          /**< type of the message */
  uint8_t frame_count[sizeof(uint32_t)]; /**< total number of frames */
} qking_debugger_send_backtrace_total_t;

/**
 * Incoming message: set behaviour when exception occures.
 */
typedef struct {
  uint8_t type;   /**< type of the message */
  uint8_t enable; /**< non-zero: enable stop at exception */
} qking_debugger_receive_exception_config_t;

/**
 * Incoming message: set parser configuration.
 */
typedef struct {
  uint8_t type;        /**< type of the message */
  uint8_t enable_wait; /**< non-zero: wait after parsing is completed */
} qking_debugger_receive_parser_config_t;

/**
 * Incoming message: get backtrace.
 */
typedef struct {
  uint8_t type;                        /**< type of the message */
  uint8_t min_depth[sizeof(uint32_t)]; /**< minimum depth*/
  uint8_t max_depth[sizeof(uint32_t)]; /**< maximum depth (0 - unlimited) */
  uint8_t get_total_frame_count; /**< non-zero: if total frame count is also
                                    requested */
} qking_debugger_receive_get_backtrace_t;

/**
 * Incoming message: first message of evaluating expression.
 */
typedef struct {
  uint8_t type;                        /**< type of the message */
  uint8_t eval_size[sizeof(uint32_t)]; /**< total size of the message */
} qking_debugger_receive_eval_first_t;

/**
 * Incoming message: get scope variables
 */
typedef struct {
  uint8_t type;                          /**< type of the message */
  uint8_t chain_index[sizeof(uint32_t)]; /**< index element of the scope */
} qking_debugger_receive_get_scope_variables_t;

/**
 * Incoming message: first message of client source.
 */
typedef struct {
  uint8_t type;                        /**< type of the message */
  uint8_t code_size[sizeof(uint32_t)]; /**< total size of the message */
} qking_debugger_receive_client_source_first_t;

void qking_debugger_free_unreferenced_byte_code(void);

bool qking_debugger_receive(qking_debugger_uint8_data_t **message_data_p);

void qking_debugger_breakpoint_hit(uint8_t message_type);

void qking_debugger_send_type(qking_debugger_header_type_t type);
bool qking_debugger_send_configuration(uint8_t max_message_size);
void qking_debugger_send_data(qking_debugger_header_type_t type,
                              const void *data, size_t size);
bool qking_debugger_send_string(uint8_t message_type, uint8_t sub_type,
                                const uint8_t *string_p, size_t string_length);
bool qking_debugger_send_function_cp(qking_debugger_header_type_t type,
                                     ecma_compiled_code_t *compiled_code_p);
bool qking_debugger_send_parse_function(uint32_t line, uint32_t column);
void qking_debugger_send_memstats(void);
bool qking_debugger_send_exception_string(void);

#endif /* QKING_DEBUGGER */

#endif /* !DEBUGGER_H */
