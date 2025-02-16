#include "Tactility/StringUtils.h"
#include "Tactility/hal/gps/GpsDevice.h"
#include <Tactility/Log.h>
#include <Tactility/hal/uart/Uart.h>
#include <Tactility/kernel/Kernel.h>
#include "Tactility/service/gps/Ublox.h"
#include <cstring>

#define TAG "gps"
#define GPS_UART_BUFFER_SIZE 256

using namespace tt;
using namespace tt::hal;

namespace tt::hal::gps {

bool readMessagesUntil(uart_port_t port, uint8_t* buffer, size_t bufferSize, uint8_t untilByte, bool appendNullTerminator, const std::function<bool(const uint8_t*)>& onMessage, TickType_t duration) {
    TickType_t end_time = kernel::getTicks() + duration;
    TickType_t now;

    do {
        now = kernel::getTicks();
        if (now > end_time) {
            break;
        } else {
            TickType_t time_left = end_time - now;

            if (uart::readUntil(port, (uint8_t*)buffer, bufferSize, untilByte, time_left, appendNullTerminator)) {
                if (onMessage(buffer)) {
                    return true;
                }
            }
        }
    } while (kernel::getTicks() < end_time);

    return false;
}

inline bool isUbloxHardwareString(const std::string& input) {
    return input.length() == 8 && string::isAsciiHexString(input);
}

bool initGpsM10Q(uart_port_t port, GpsInfo& info) {
    uint8_t buffer[GPS_UART_BUFFER_SIZE];

    uart::flushInput(port);

    // "34.8 MON-VER (0x0A 0x04)" https://cdn.sparkfun.com/datasheets/Sensors/GPS/760.pdf
    uint8_t get_version_command[] = { 0xB5, 0x62, 0x0a, 0x04, 0x00, 0x00, 0x0e, 0x34 };
    if (uart::writeBytes(port, get_version_command, sizeof(get_version_command)) != sizeof(get_version_command)) {
        TT_LOG_W(TAG, "Failed to fetch version");
    }

    std::string line;
    readMessagesUntil(
        port,
        buffer,
        sizeof(buffer),
        0x00U,
        false,
        [&info, &line](const uint8_t* data) {
            const char* data_str = (const char*)data;
            line.assign(data_str); // safe copy
            if (line.starts_with("ROM ")) {
                info.software = line;
            } else if (line.starts_with("FWVER=")) {
                info.firmwareVersion = line.substr(6);
            } else if (line.starts_with("PROTVER=")) {
                info.protocolVersion = line.substr(8);
            } else if (line.starts_with("MOD=")) {
                info.module = line.substr(4);
            } else if (isUbloxHardwareString(line)) {
                info.hardware = line;
            } else if (line.contains(";")) {
                info.additional.push_back(line);
            }

            return info.hasAnyData() && line.starts_with('$'); // Stop when reading "normal" input again
        },
        250 / portTICK_PERIOD_MS
    );

    if (info.hasAnyData()) {
        TT_LOG_I(TAG, "Init complete");
        return true;
    } else {
        TT_LOG_E(TAG, "Init failed: failed to fetch info");
        return false;
    }
}

} // namespace tt::hal::gps
