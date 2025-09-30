#pragma once

#include <array>
#include <memory>

#ifdef ESP_PLATFORM
#include "LogEsp.h"
#else
#include "LogSimulator.h"
#endif

#include "LogMessages.h"
#include "LogCommon.h"

namespace tt {

struct LogEntry {
    LogLevel level = LogLevel::Verbose;
    char tag[TT_LOG_TAG_SIZE] = { 0 };
    char message[TT_LOG_MESSAGE_SIZE] = { 0 };
};

/** Make a copy of the currently stored entries.
 * The array size is TT_LOG_ENTRY_COUNT
 * @param[out] outIndex the write index for the next log entry.
 */
std::unique_ptr<std::array<LogEntry, TT_LOG_ENTRY_COUNT>> copyLogEntries(std::size_t& outIndex);

} // namespace tt
