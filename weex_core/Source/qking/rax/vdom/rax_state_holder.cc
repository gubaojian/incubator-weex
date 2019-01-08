//
// Created by Xu Jiacheng on 2018-12-29.
//

#include "rax_state_holder.h"

RAX_NAME_SPACE_BEGIN

qking_value_t RaxStateHolder::state_queue_pop() {
  qking_value_t ret = qking_acquire_value(state_queue_.front()->get());
  state_queue_.pop();
  return ret;
}

RAX_NAME_SPACE_END
