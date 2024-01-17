#ifndef ESP_PLATFORM

#include "log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>

static char tt_loglevel_to_prefix(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR:
            return 'E';
        case LOG_LEVEL_WARNING:
            return 'W';
        case LOG_LEVEL_INFO:
            return 'I';
        case LOG_LEVEL_DEBUG:
            return 'D';
        case LOG_LEVEL_TRACE:
            return 'T';
        default:
            return '?';
    }
}

static const char* tt_loglevel_to_colour(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR:
            return "\033[1;31m";
        case LOG_LEVEL_WARNING:
            return "\033[33m";
        case LOG_LEVEL_INFO:
            return "\033[32m";
        case LOG_LEVEL_DEBUG:
            return "\033[1;37m";
        case LOG_LEVEL_TRACE:
            return "\033[37m";
        default:
            return "";
    }
}

uint32_t tt_log_timestamp(void) {
    if (unlikely(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)) {
        return clock() / CLOCKS_PER_SEC * 1000;
    }
    static uint32_t base = 0;
    if (base == 0 && xPortGetCoreID() == 0) {
        base = clock() / CLOCKS_PER_SEC * 1000;
    }
    TickType_t tick_count = xPortInIsrContext() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    return base + tick_count * (1000 / configTICK_RATE_HZ);
}

void tt_log(LogLevel level, const char* tag, const char* format, ...) {
    printf(
        "%s%c (%lu) %s: ",
        tt_loglevel_to_colour(level),
        tt_loglevel_to_prefix(level),
        tt_log_timestamp(),
        tag
    );

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m\n");
}

#endif