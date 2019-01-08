#ifndef CONFIG_DISABLE_COMPILER_BUILTIN

#include "include/qking.h"
#include <string>
#include "base/common_logger.h"
#include "core/include/qking_core.h"
#include "core/jcontext/jcontext.h"
#include "core/lit/lit_strings.h"
#include "core/parser/parser_exec_state.h"
#include "core/vm/vm_exec_state.h"
#include "decoder/exec_state_decode.h"
#include "ecma_helpers.h"
#include "handler/handler.h"
#include "stdbool.h"
#include "stdlib.h"

/**
 * error_value must be checked and released after this call;
 */
bool qking_set_compile_code(qking_executor_t executor, const char *pstr,
                            qking_value_t *error_value) {
  using namespace quick::king;
  bool finished = false;
  do {
    std::string error;
    std::string input = pstr;
    ExecState *engine_p = new ExecState((qking_vm_exec_state_t *)executor);
    std::string err;
    json12::Json json = json12::Json::parse(input, err);
    if (!err.empty() || json.is_null()) {
      engine_p->context<VMContext>()->raw_source() = input;
    } else {
      engine_p->context<VMContext>()->raw_json() = json;
    }
    engine_p->Compile(error);
    if (!error.empty()) {
      const char *es = error.c_str();
      LOGE("%s", es);
      if (error_value) {
        *error_value = qking_create_string_sz(
            (const qking_char_t *)es, static_cast<qking_size_t>(error.size()));
      }
      break;
    }
    delete engine_p;
    finished = true;

  } while (0);

  return finished;
}

#endif
