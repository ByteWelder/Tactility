#ifdef ESP_PLATFORM

#include "Tactility/LogCommon.h"
#include <esp_log.h>

namespace tt {
void storeLog(LogLevel level, const char* format, va_list args);
}

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
