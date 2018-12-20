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

#ifndef QKING_COMPILER_COMMON_LOGGER_H
#define QKING_COMPILER_COMMON_LOGGER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(__ANDROID__)

#include <android/log.h>

#endif  //__ANDROID__

#ifdef __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
#define QKING_PLATFORM_IOS
#elif TARGET_OS_IPHONE
#define QKING_PLATFORM_IOS
#elif TARGET_OS_MAC
#define QKING_PLATFORM_MAC
#else
#define QKING_PLATFORM_MAC
#endif
#endif

#ifndef QKING_NDEBUG
#define DEBUG_LOG_ENABLE

#if defined(__ANDROID__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "QKING_C", __VA_ARGS__)
#else
#define LOGD(...) qking_common_logger_logd(__VA_ARGS__)
#endif

#else
#define LOGD(...) ((void)0)
#endif

#if defined(__ANDROID__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "QKING_C", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "QKING_C", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "QKING_C", __VA_ARGS__)
#else
#define LOGI(...) qking_common_logger_logi(__VA_ARGS__)
#define LOGW(...) qking_common_logger_logw(__VA_ARGS__)
#define LOGE(...) qking_common_logger_loge(__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __ANDROID__

bool qking_common_logger_is_log_on(void);

void qking_common_logger_set_log_on(bool flag);

void qking_common_logger_logd(const char *fmt, ...);

void qking_common_logger_logi(const char *fmt, ...);

void qking_common_logger_logw(const char *fmt, ...);

void qking_common_logger_loge(const char *fmt, ...);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // QKING_COMPILER_COMMON_LOGGER_H
