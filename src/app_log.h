#ifndef APP_LOG_H
#define APP_LOG_H

#ifdef IS_HOST_TEST

#include <stdio.h>

#define APP_HOST_LOG(level, tag, ...)                                      \
    do {                                                                    \
        fprintf(stderr, "%s (%s): ", (level), (tag));                     \
        fprintf(stderr, __VA_ARGS__);                                       \
        fputc('\n', stderr);                                                \
    } while (0)

#define APP_LOGI(tag, ...) APP_HOST_LOG("I", (tag), __VA_ARGS__)
#define APP_LOGW(tag, ...) APP_HOST_LOG("W", (tag), __VA_ARGS__)
#define APP_LOGE(tag, ...) APP_HOST_LOG("E", (tag), __VA_ARGS__)

#else

#include "esp_log.h"

#define APP_LOGI ESP_LOGI
#define APP_LOGW ESP_LOGW
#define APP_LOGE ESP_LOGE

#endif

#endif
