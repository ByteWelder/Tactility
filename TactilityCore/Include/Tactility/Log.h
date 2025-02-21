#pragma once

#include "LogMessages.h"
#include <array>
#include <memory>

#if not defined(ESP_PLATFORM) or (defined(CONFIG_SPIRAM_USE_MALLOC) && CONFIG_SPIRAM_USE_MALLOC  == 1)
#define TT_LOG_ENTRY_COUNT 200
#define TT_LOG_MESSAGE_SIZE 128
#else
#define TT_LOG_ENTRY_COUNT 50
#define TT_LOG_MESSAGE_SIZE 50
#endif

#ifdef ESP_PLATFORM
#include "LogEsp.h"
#else
#include "LogSimulator.h"
#endif

#include "LogCommon.h"

namespace tt {

struct LogEntry {
    LogLevel level = LogLevel::Verbose;
    char message[TT_LOG_MESSAGE_SIZE] = { 0 };
};

/** Make a copy of the currently stored entries.
 * The array size is TT_LOG_ENTRY_COUNT
 * @param[out] outIndex the write index for the next log entry.
 */
std::unique_ptr<std::array<LogEntry, TT_LOG_ENTRY_COUNT>> copyLogEntries(std::size_t& outIndex);

} // namespace tt
