#pragma once

namespace tt {

/** Used for log output filtering */
enum class LogLevel : int {
    Error, /*!< Critical errors, software module can not recover on its own */
    Warning, /*!< Error conditions from which recovery measures have been taken */
    Info, /*!< Information messages which describe normal flow of events */
    Debug, /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    Verbose /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
};

#if not defined(ESP_PLATFORM) or (defined(CONFIG_SPIRAM_USE_MALLOC) && CONFIG_SPIRAM_USE_MALLOC  == 1)
constexpr auto TT_LOG_TAG_SIZE = 16;
constexpr auto TT_LOG_MESSAGE_SIZE = 128;
constexpr auto TT_LOG_ENTRY_COUNT = 200;
constexpr auto TT_LOG_STORAGE_LEVEL = LogLevel::Info;
#else
constexpr auto TT_LOG_TAG_SIZE = 16;
constexpr auto TT_LOG_MESSAGE_SIZE = 50;
constexpr auto TT_LOG_ENTRY_COUNT = 16;
constexpr auto TT_LOG_STORAGE_LEVEL = LogLevel::Warning;
#endif

}