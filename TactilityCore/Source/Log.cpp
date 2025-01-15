#include "Mutex.h"
#include <cstring>
#include <sstream>

namespace tt {

static LogEntry* logEntries = nullptr;
static unsigned int nextLogEntryIndex;
static Mutex logMutex;

static void ensureLogEntriesExist() {
    if (logEntries == nullptr) {
        logEntries = new LogEntry[TT_LOG_ENTRY_COUNT];
        assert(logEntries != nullptr);
        nextLogEntryIndex = 0;
    }
}

static void storeLog(LogLevel level, const char* format, va_list args) {
    if (logMutex.lock(5 / portTICK_PERIOD_MS)) {
        ensureLogEntriesExist();

        logEntries[nextLogEntryIndex].level = level;
        vsnprintf(logEntries[nextLogEntryIndex].message, TT_LOG_MESSAGE_SIZE, format, args);

        nextLogEntryIndex++;
        if (nextLogEntryIndex == TT_LOG_ENTRY_COUNT) {
            nextLogEntryIndex = 0;
        }

        logMutex.unlock();
    }
}

LogEntry* copyLogEntries(unsigned int& outIndex) {
    if (logMutex.lock(5 / portTICK_PERIOD_MS)) {
        auto* newEntries = new LogEntry[TT_LOG_ENTRY_COUNT];
        assert(newEntries != nullptr);
        for (int i = 0; i < TT_LOG_ENTRY_COUNT; ++i) {
            memcpy(&newEntries[i], &logEntries[i], sizeof(LogEntry));
        }
        outIndex = nextLogEntryIndex;
        logMutex.unlock();
        return newEntries;
    } else {
        return nullptr;
    }
}

} // namespace tt

#ifndef ESP_PLATFORM

#include "Log.h"

#include <cstdint>
#include <sys/time.h>
#include <sstream>

namespace tt {

static char toPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::Error:
            return 'E';
        case LogLevel::Warning:
            return 'W';
        case LogLevel::Info:
            return 'I';
        case LogLevel::Debug:
            return 'D';
        case LogLevel::Verbose:
            return 'T';
        default:
            return '?';
    }
}

static const char* toColour(LogLevel level) {
    switch (level) {
        case LogLevel::Error:
            return "\033[1;31m";
        case LogLevel::Warning:
            return "\033[33m";
        case LogLevel::Info:
            return "\033[32m";
        case LogLevel::Debug:
            return "\033[1;37m";
        case LogLevel::Verbose:
            return "\033[37m";
        default:
            return "";
    }
}

static uint64_t getTimestamp() {
#ifdef ESP_PLATFORM
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return clock() / CLOCKS_PER_SEC * 1000;
    }
    static uint32_t base = 0;
    if (base == 0 && xPortGetCoreID() == 0) {
        base = clock() / CLOCKS_PER_SEC * 1000;
    }
    TickType_t tick_count = xPortInIsrContext() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    return base + tick_count * (1000 / configTICK_RATE_HZ);
#else
    static uint64_t base = 0;

    struct timeval time {};
    gettimeofday(&time, nullptr);
    uint64_t now = ((uint64_t)time.tv_sec * 1000) + (time.tv_usec / 1000);
    if (base == 0) {
        base = now;
    }
    return now - base;
#endif
}

void log(LogLevel level, const char* tag, const char* format, ...) {
    std::stringstream buffer;
    buffer << toColour(level) << toPrefix(level) << " (" << getTimestamp() << ") " << tag << ": " << format << "\033[0m\n";

    va_list args;
    va_start(args, format);
    vprintf(buffer.str().c_str(), args);
    va_end(args);

    va_start(args, format);
    tt::storeLog(level, buffer.str().c_str(), args);
    va_end(args);
}

} // namespace

#else // ESP_PLATFORM

#include <esp_log.h>

extern "C" {

extern void __real_esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...);

void __wrap_esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    tt::storeLog((tt::LogLevel)level, format, args);
    esp_log_writev(level, tag, format, args);
    va_end(args);
}

}

#endif

