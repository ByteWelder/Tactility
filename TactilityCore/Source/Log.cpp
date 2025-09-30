#include "Tactility/Mutex.h"

#include <cstring>
#include <sstream>

namespace tt {

static std::array<LogEntry, TT_LOG_ENTRY_COUNT> logEntries;
static size_t nextLogEntryIndex;

/**
 * This used to be a simple static value, but that crashes on device boot where early logging happens.
 * For some unknown reason, the static Mutex instance wouldn't have their constructor called before
 * the mutex is used.
 */
Mutex& getLogMutex() {
    static Mutex* logMutex = nullptr;
    if (logMutex == nullptr) {
        logMutex = new Mutex();
    }
    return *logMutex;
}

void storeLog(LogLevel level, const char* tag, const char* format, ...) {
    if (TT_LOG_STORAGE_LEVEL >= level && getLogMutex().lock(5 / portTICK_PERIOD_MS)) {
        logEntries[nextLogEntryIndex].level = level;
        va_list args;
        va_start(args, format);
        vsnprintf(logEntries[nextLogEntryIndex].message, TT_LOG_MESSAGE_SIZE, format, args);
        va_end(args);
        strncpy(logEntries[nextLogEntryIndex].tag, tag, TT_LOG_TAG_SIZE);

        nextLogEntryIndex++;
        if (nextLogEntryIndex == TT_LOG_ENTRY_COUNT) {
            nextLogEntryIndex = 0;
        }

        getLogMutex().unlock();
    }
}

std::unique_ptr<std::array<LogEntry, TT_LOG_ENTRY_COUNT>> copyLogEntries(std::size_t& outIndex) {
    if (getLogMutex().lock(5 / portTICK_PERIOD_MS)) {
        auto copy = std::make_unique<std::array<LogEntry, TT_LOG_ENTRY_COUNT>>(logEntries);
        getLogMutex().unlock();
        outIndex = nextLogEntryIndex;
        return copy;
    } else {
        return nullptr;
    }
}


} // namespace tt
