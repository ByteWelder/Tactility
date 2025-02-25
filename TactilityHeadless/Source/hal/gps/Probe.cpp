#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/gps/Ublox.h"
#include <Tactility/Log.h>
#include <Tactility/hal/uart/Uart.h>
#include <Tactility/kernel/Kernel.h>
#include <cstring>

#define TAG "gps"
#define GPS_UART_BUFFER_SIZE 256

using namespace tt;
using namespace tt::hal;

namespace tt::hal::gps {

/**
 * From: https://github.com/meshtastic/firmware/blob/3b0232de1b6282eacfbff6e50b68fca7e67b8511/src/meshUtils.cpp#L40
 */
char* strnstr(const char* s, const char* find, size_t slen) {
    char c;
    if ((c = *find++) != '\0') {
        char sc;
        size_t len;

        len = strlen(find);
        do {
            do {
                if (slen-- < 1 || (sc = *s++) == '\0')
                    return (nullptr);
            } while (sc != c);
            if (len > slen)
                return (nullptr);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char*)s);
}

/**
 * From: https://github.com/meshtastic/firmware/blob/f81d3b045dd1b7e3ca7870af3da915ff4399ea98/src/gps/GPS.cpp
 */
GpsResponse getAck(uart::Uart& uart, const char* message, uint32_t waitMillis) {
    uint8_t buffer[768] = {0};
    uint8_t b;
    int bytesRead = 0;
    uint32_t startTimeout = kernel::getMillis() + waitMillis;
#ifdef GPS_DEBUG
    std::string debugmsg = "";
#endif
    while (kernel::getMillis() < startTimeout) {
        if (uart.available()) {
            uart.readByte(&b);

#ifdef GPS_DEBUG
            debugmsg += vformat("%c", (b >= 32 && b <= 126) ? b : '.');
#endif
            buffer[bytesRead] = b;
            bytesRead++;
            if ((bytesRead == 767) || (b == '\r')) {
                if (strnstr((char*)buffer, message, bytesRead) != nullptr) {
#ifdef GPS_DEBUG
                    LOG_DEBUG("Found: %s", message); // Log the found message
#endif
                    return GpsResponse::Ok;
                } else {
                    bytesRead = 0;
#ifdef GPS_DEBUG
                    LOG_DEBUG(debugmsg.c_str());
#endif
                }
            }
        }
    }
    return GpsResponse::None;
}

/**
 * From: https://github.com/meshtastic/firmware/blob/f81d3b045dd1b7e3ca7870af3da915ff4399ea98/src/gps/GPS.cpp
 */
#define PROBE_SIMPLE(UART, CHIP, TOWRITE, RESPONSE, DRIVER, TIMEOUT, ...) \
    do {                                                                  \
        TT_LOG_I(TAG, "Probing for %s (%s)", CHIP, TOWRITE);              \
        UART.flushInput();                                           \
        UART.writeString(TOWRITE "\r\n", TIMEOUT);                 \
        if (getAck(UART, RESPONSE, TIMEOUT) == GpsResponse::Ok) {        \
            TT_LOG_I(TAG, "Probe detected %s %s", CHIP, #DRIVER);         \
            return DRIVER;                                                \
        }                                                                 \
    } while (0)

/**
 * From: https://github.com/meshtastic/firmware/blob/f81d3b045dd1b7e3ca7870af3da915ff4399ea98/src/gps/GPS.cpp
 */
GpsModel probe(uart::Uart& uart) {
    // Close all NMEA sentences, valid for L76K, ATGM336H (and likely other AT6558 devices)
    uart.writeString("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
    kernel::delayMillis(20);

    // Close NMEA sequences on Ublox
    uart.writeString("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
    uart.writeString("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
    uart.writeString("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
    kernel::delayMillis(20);

    // Unicore UFirebirdII Series: UC6580, UM620, UM621, UM670A, UM680A, or UM681A
    PROBE_SIMPLE(uart, "UC6580", "$PDTINFO", "UC6580", GpsModel::UC6580, 500);
    PROBE_SIMPLE(uart, "UM600", "$PDTINFO", "UM600", GpsModel::UC6580, 500);
    PROBE_SIMPLE(uart, "ATGM336H", "$PCAS06,1*1A", "$GPTXT,01,01,02,HW=ATGM336H", GpsModel::ATGM336H, 500);

    /* ATGM332D series (-11(GPS), -21(BDS), -31(GPS+BDS), -51(GPS+GLONASS), -71-0(GPS+BDS+GLONASS))
    based on AT6558 */
    PROBE_SIMPLE(uart, "ATGM332D", "$PCAS06,1*1A", "$GPTXT,01,01,02,HW=ATGM332D", GpsModel::ATGM336H, 500);

    /* Airoha (Mediatek) AG3335A/M/S, A3352Q, Quectel L89 2.0, SimCom SIM65M */
    uart.writeString("$PAIR062,2,0*3C\r\n"); // GSA OFF to reduce volume
    uart.writeString("$PAIR062,3,0*3D\r\n"); // GSV OFF to reduce volume
    uart.writeString("$PAIR513*3D\r\n"); // save configuration
    PROBE_SIMPLE(uart, "AG3335", "$PAIR021*39", "$PAIR021,AG3335", GpsModel::AG3335, 500);
    PROBE_SIMPLE(uart, "AG3352", "$PAIR021*39", "$PAIR021,AG3352", GpsModel::AG3352, 500);
    PROBE_SIMPLE(uart, "LC86", "$PQTMVERNO*58", "$PQTMVERNO,LC86", GpsModel::AG3352, 500);

    PROBE_SIMPLE(uart, "L76K", "$PCAS06,0*1B", "$GPTXT,01,01,02,SW=", GpsModel::MTK, 500);

    // Close all NMEA sentences, valid for L76B MTK platform (Waveshare Pico GPS)
    uart.writeString("$PMTK514,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*2E\r\n");
    kernel::delayMillis(20);

    PROBE_SIMPLE(uart, "L76B", "$PMTK605*31", "Quectel-L76B", GpsModel::MTK_L76B, 500);
    PROBE_SIMPLE(uart, "PA1616S", "$PMTK605*31", "1616S", GpsModel::MTK_PA1616S, 500);

    auto ublox_result = ublox::probe(uart);
    if (ublox_result != GpsModel::Unknown) {
        return ublox_result;
    } else {
        TT_LOG_W(TAG, "No GNSS Module (baudrate %lu)", uart.getBaudRate());
        return GpsModel::Unknown;
    }
}

} // namespace tt::hal::gps
