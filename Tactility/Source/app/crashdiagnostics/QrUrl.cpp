#ifdef ESP_PLATFORM

#include "Tactility/app/crashdiagnostics/QrUrl.h"

#include <Tactility/kernel/PanicHandler.h>

#include <sstream>
#include <esp_cpu_utils.h>

#include <sdkconfig.h>

std::string getUrlFromCrashData() {
    auto crash_data = getRtcCrashData();
    auto* stack_buffer = (uint32_t*) malloc(crash_data.callstackLength * 2 * sizeof(uint32_t));
    for (int i = 0; i < crash_data.callstackLength; ++i) {
        const CallstackFrame&frame = crash_data.callstack[i];
        uint32_t pc = esp_cpu_process_stack_pc(frame.pc);
#if CRASH_DATA_INCLUDES_SP
        uint32_t sp = frame.sp;
#endif
        stack_buffer[i * 2] = pc;
#if CRASH_DATA_INCLUDES_SP
        stack_buffer[(i * 2) + 1] = sp;
#endif
    }

    std::stringstream stream;

    stream << "https://oops.tactility.one";
    stream << "?v=" << TT_VERSION; // Version
    stream << "&a=" << CONFIG_IDF_TARGET; // Architecture
    stream << "&b=" << CONFIG_TT_DEVICE_ID; // Board identifier
    stream << "&s="; // Stacktrace

    for (int i = crash_data.callstackLength - 1; i >= 0; --i) {
        uint32_t pc = stack_buffer[(i * 2)];
        stream << std::hex << pc;
#if CRASH_DATA_INCLUDES_SP
        uint32_t sp = stack_buffer[(i * 2) + 1];
        stream << std::hex << sp;
#endif
    }

    free(stack_buffer);

    return stream.str();
}

#endif
