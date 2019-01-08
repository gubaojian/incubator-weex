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
//
// Created by Xu Jiacheng on 2018/9/15.
//

#include "common_logger.h"

#ifdef DEBUG_LOG_ENABLE
static bool g_log_enabled_ = true;
#else
static bool g_log_enabled_ = false;
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#ifndef __ANDROID__

#ifndef QKING_PLATFORM_IOS
#define PRINT_COLOR(color)\
  printf(color);
#else
#define PRINT_COLOR(color)\
  ((void)0)
#endif

#ifndef QKING_PLATFORM_IOS
#define PRINT_RESET()\
  printf("\n" ANSI_COLOR_RESET)
#else
#define PRINT_RESET()\
  printf("\n")
#endif

bool qking_common_logger_is_log_on(){
  return g_log_enabled_;
}

void qking_common_logger_logd(const char *fmt, ...){
  if (!qking_common_logger_is_log_on()) {
    return;
  }

  va_list args;
  va_start(args, fmt);

#ifdef QKING_PLATFORM_IOS
  printf("QKING[D]-");
#endif
  vprintf(fmt, args);
  printf("\n");

  va_end(args);
}


void qking_common_logger_logi(const char *fmt, ...){
  va_list args;
  va_start(args, fmt);

#ifdef QKING_PLATFORM_IOS
  printf("QKING[I]-");
#endif
  PRINT_COLOR(ANSI_COLOR_BLUE);
  vprintf(fmt, args);
  PRINT_RESET();

  va_end(args);
}


void qking_common_logger_logw(const char *fmt, ...){
  va_list args;
  va_start(args, fmt);

#ifdef QKING_PLATFORM_IOS
  printf("QKING[W]-");
#endif
  PRINT_COLOR(ANSI_COLOR_YELLOW);
  vprintf(fmt, args);
  PRINT_RESET();

  va_end(args);
}


void qking_common_logger_loge(const char *fmt, ...){
  va_list args;
  va_start(args, fmt);

#ifdef QKING_PLATFORM_IOS
  printf("QKING[W]-");
#endif
  PRINT_COLOR(ANSI_COLOR_RED);
  vprintf(fmt, args);
  PRINT_RESET();

  va_end(args);
}

void qking_common_logger_set_log_on(bool flag) {
  g_log_enabled_ = flag;
}

#endif
