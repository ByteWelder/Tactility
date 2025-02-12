#include <Tactility/Log.h>
#include <Tactility/hal/uart/Uart.h>
#include <Tactility/kernel/Kernel.h>

#define TAG "gps"
#define GPS_UART_BUFFER_SIZE 256

using namespace tt;
using namespace tt::hal;

namespace tt::hal::gps {

static int ackGps(uart_port_t port, uint8_t* buffer, uint16_t size, uint8_t requestedClass, uint8_t requestedId) {
    uint16_t ubxFrameCounter = 0;
    uint32_t startTime = kernel::getTicks();
    uint16_t needRead;

    while (kernel::getTicks() - startTime < 800) {
        while (uart::available(port)) {
            uint8_t c;
            uart::readByte(port, &c);
            switch (ubxFrameCounter) {
                case 0:
                    if (c == 0xB5) {
                        ubxFrameCounter++;
                    }
                    break;
                case 1:
                    if (c == 0x62) {
                        ubxFrameCounter++;
                    } else {
                        ubxFrameCounter = 0;
                    }
                    break;
                case 2:
                    if (c == requestedClass) {
                        ubxFrameCounter++;
                    } else {
                        ubxFrameCounter = 0;
                    }
                    break;
                case 3:
                    if (c == requestedId) {
                        ubxFrameCounter++;
                    } else {
                        ubxFrameCounter = 0;
                    }
                    break;
                case 4:
                    needRead = c;
                    ubxFrameCounter++;
                    break;
                case 5:
                    needRead |= (c << 8);
                    ubxFrameCounter++;
                    break;
                case 6:
                    if (needRead >= size) {
                        ubxFrameCounter = 0;
                        break;
                    }
                    if (uart::readBytes(port, buffer, needRead) != needRead) {
                        ubxFrameCounter = 0;
                    } else {
                        return needRead;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    return 0;
}

static bool configureGps(uart_port_t port, uint8_t* buffer, size_t bufferSize) {
    // According to the reference implementation, L76K GPS uses 9600 baudrate, but the default in the developer device was 38400
    // https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/GPSShield/GPSShield.ino
    bool result = false;
    uint32_t startTimeout;
    for (int i = 0; i < 3; ++i) {
        if (!uart::writeString(port, "$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n")) {
            TT_LOG_E(TAG, "Failed to write init string");
        }

        kernel::delayMillis(5U);
        // Get version information
        startTimeout = kernel::getMillis() + 1000;

        TT_LOG_I(TAG, "Manual flushing of input");
#ifdef ESP_PLATFORM
        esp_rom_printf("Waiting...");
#endif
        while (uart::available(port)) {
#ifdef ESP_PLATFORM
            esp_rom_printf(".");
#endif
            uart::readUntil(port, buffer, bufferSize, '\n');
            if (kernel::getMillis() > startTimeout) {
                TT_LOG_E(TAG, "NMEA timeout");
                return false;
            }
        };
#ifdef ESP_PLATFORM
        esp_rom_printf("\n");
#endif
        uart::flushInput(port);
        kernel::delayMillis(200);

        if (!uart::writeString(port, "$PCAS06,0*1B\r\n")) {
            TT_LOG_E(TAG, "Failed to write PCAS06");
        }

        startTimeout = kernel::getMillis() + 500;
        while (!uart::available(port)) {
            if (kernel::getMillis() > startTimeout) {
                TT_LOG_E(TAG, "L76K timeout");
                return false;
            }
        }
        auto ver = uart::readStringUntil(port, '\n');
        if (ver.starts_with("$GPTXT,01,01,02")) {
            TT_LOG_I(TAG, "L76K GNSS init success");
            result = true;
            break;
        }
        kernel::delayMillis(500);
    }
    // Initialize the L76K Chip, use GPS + GLONASS
    uart::writeString(port, "$PCAS04,5*1C\r\n");
    kernel::delayMillis(250);
    uart::writeString(port, "$PCAS03,1,1,1,1,1,1,1,1,1,1,,,0,0*26\r\n");
    kernel::delayMillis(250);

    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    uart::writeString(port, "$PCAS11,3*1E\r\n");

    return result;
}

static bool recoverGps(uart_port_t port, uint8_t* buffer, size_t bufferSize) {
    uint8_t cfg_clear1[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1C, 0xA2};
    uint8_t cfg_clear2[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1B, 0xA1};
    uint8_t cfg_clear3[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x03, 0x1D, 0xB3};

    if (!uart::writeBytes(port, cfg_clear1, sizeof(cfg_clear1), 10)) {
        return false;
        TT_LOG_E(TAG, "Failed to send ack 1");
    }

    if (ackGps(port, buffer, bufferSize, 0x05, 0x01)) {
        TT_LOG_I(TAG, "Ack 1");
    } else {
        TT_LOG_W(TAG, "Ack 1 failed");
    }

    if (!uart::writeBytes(port, cfg_clear2, sizeof(cfg_clear2))) {
        return false;
        TT_LOG_E(TAG, "Failed to send ack 2");
    }

    if (ackGps(port, buffer, bufferSize, 0x05, 0x01)) {
        TT_LOG_I(TAG, "Ack 2");
    } else {
        TT_LOG_W(TAG, "Ack 2 failed");
    }

    if (!uart::writeBytes(port, cfg_clear3, sizeof(cfg_clear3))) {
        TT_LOG_E(TAG, "Failed to send ack 3");
        return false;
    }

    if (ackGps(port, buffer, bufferSize, 0x05, 0x01)) {
        TT_LOG_I(TAG, "Ack 3");
    } else {
        TT_LOG_W(TAG, "Ack 3 failed");
    }

    // UBX-CFG-RATE, Size 8, 'Navigation/measurement rate settings'
    uint8_t cfg_rate[] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
    uart::writeBytes(port, cfg_rate, sizeof(cfg_rate));
    if (ackGps(port, buffer, bufferSize, 0x06, 0x08)) {
        TT_LOG_I(TAG, "Ack completed");
    } else {
        return false;
    }
    return true;
}

bool initGpsDefault(uart_port_t port) {
    uint8_t buffer[GPS_UART_BUFFER_SIZE];
    if (!configureGps(port, buffer, GPS_UART_BUFFER_SIZE)) {
        if (!recoverGps(port, buffer, GPS_UART_BUFFER_SIZE)) {
            uint32_t initial_baud_rate = std::max(uart::getBaudRate(port), (uint32_t)9600U);
            uart::setBaudRate(port, 9600U);
            if (!recoverGps(port, buffer, GPS_UART_BUFFER_SIZE)) {
                TT_LOG_E(TAG, "Recovery repeatedly failed");
                return false;
            } else {
                TT_LOG_I(TAG, "Recovery 2 complete");
            }
            uart::setBaudRate(port, initial_baud_rate);
        } else {
            TT_LOG_I(TAG, "Recovery 1 complete");
        }
    }

    TT_LOG_I(TAG, "Init complete");

    return true;
}

} // namespace tt::hal::gps
