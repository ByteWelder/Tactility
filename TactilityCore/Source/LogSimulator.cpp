#ifndef ESP_PLATFORM

#include <Tactility/Log.h>
#include <Tactility/LoggerAdapterShared.h>
#include <Tactility/LoggerAdapterGeneric.h>

#include <iomanip>
#include <sstream>

namespace tt {

void log(LogLevel level, const char* tag, const char* format, ...) {
    constexpr auto COLOR_RESET = "\033[0m";
    constexpr auto COLOR_GREY = "\033[37m";
    std::stringstream buffer;
    buffer << COLOR_GREY << getLogTimestamp() << " [" << toTagColour(level) << toPrefix(level) << COLOR_GREY << "] [" << COLOR_RESET << tag << COLOR_GREY << "] " << toMessageColour(level) << format  << COLOR_RESET << std::endl;
    va_list args;
    va_start(args, format);
    vprintf(buffer.str().c_str(), args);
    va_end(args);
}

} // namespace tt

#endif