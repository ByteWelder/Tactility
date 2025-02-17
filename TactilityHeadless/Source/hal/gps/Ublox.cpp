#include "Tactility/hal/gps/Ublox.h"
#include "Tactility/hal/gps/UbloxMessages.h"
#include <cstring>

#define TAG "ublox"

namespace tt::hal::gps::ublox {

bool initUblox6(uart_port_t port);
bool initUblox789(uart_port_t port);
bool initUblox10(uart_port_t port);

#define SEND_UBX_PACKET(PORT, BUFFER, TYPE, ID, DATA, ERRMSG, TIMEOUT)                                                                         \
    do {                                                                                                                         \
        auto msglen = makePacket(TYPE, ID, DATA, sizeof(DATA), BUFFER);                                                                    \
        uart::writeBytes(PORT, BUFFER, sizeof(BUFFER));                                                                                  \
        if (getAck(PORT, TYPE, ID, TIMEOUT) != GNSS_RESPONSE_OK) {                                                                     \
            TT_LOG_I(TAG, "Sending packet failed: %s", #ERRMSG);                                                                                      \
        }                                                                                                                        \
    } while (0)

void checksum(uint8_t* message, size_t length) {
    uint8_t CK_A = 0, CK_B = 0;

    // Calculate the checksum, starting from the CLASS field (which is message[2])
    for (size_t i = 2; i < length - 2; i++) {
        CK_A = (CK_A + message[i]) & 0xFF;
        CK_B = (CK_B + CK_A) & 0xFF;
    }

    // Place the calculated checksum values in the message
    message[length - 2] = CK_A;
    message[length - 1] = CK_B;
}

uint8_t makePacket(uint8_t classId, uint8_t messageId, const uint8_t* payload, uint8_t payloadSize, uint8_t* bufferOut) {
    // Construct the UBX packet
    bufferOut[0] = 0xB5U; // header
    bufferOut[1] = 0x62U; // header
    bufferOut[2] = classId; // class
    bufferOut[3] = messageId; // id
    bufferOut[4] = payloadSize; // length
    bufferOut[5] = 0x00U;

    bufferOut[6 + payloadSize] = 0x00U; // CK_A
    bufferOut[7 + payloadSize] = 0x00U; // CK_B

    for (int i = 0; i < payloadSize; i++) {
        bufferOut[6 + i] = payload[i];
    }
    checksum(bufferOut, (payloadSize + 8U));
    return (payloadSize + 8U);
}

GPS_RESPONSE getAck(uart_port_t port, uint8_t class_id, uint8_t msg_id, uint32_t waitMillis) {
    uint8_t b;
    uint8_t ack = 0;
    const uint8_t ackP[2] = {class_id, msg_id};
    uint8_t buf[10] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t startTime = kernel::getMillis();
    const char frame_errors[] = "More than 100 frame errors";
    int sCounter = 0;
#ifdef GPS_DEBUG
    std::string debugmsg = "";
#endif

    for (int j = 2; j < 6; j++) {
        buf[8] += buf[j];
        buf[9] += buf[8];
    }

    for (int j = 0; j < 2; j++) {
        buf[6 + j] = ackP[j];
        buf[8] += buf[6 + j];
        buf[9] += buf[8];
    }

    while (kernel::getTicks() - startTime < waitMillis) {
        if (ack > 9) {
#ifdef GPS_DEBUG
            TT_LOG_I(TAG, "Got ACK for class %02X message %02X in %lums", class_id, msg_id, kernel::getMillis() - startTime);
#endif
            return GNSS_RESPONSE_OK; // ACK received
        }
        if (uart::available(port)) {
            uart::readByte(port, &b);
            if (b == frame_errors[sCounter]) {
                sCounter++;
                if (sCounter == 26) {
#ifdef GPS_DEBUG

                    TT_LOG_I(TAG, "%s", debugmsg.c_str());
#endif
                    return GNSS_RESPONSE_FRAME_ERRORS;
                }
            } else {
                sCounter = 0;
            }
#ifdef GPS_DEBUG
            debugmsg += std::format("%02X", b);
#endif
            if (b == buf[ack]) {
                ack++;
            } else {
                if (ack == 3 && b == 0x00) { // UBX-ACK-NAK message
#ifdef GPS_DEBUG
                    TT_LOG_I(TAG, "%s", debugmsg.c_str());
#endif
                    TT_LOG_W(TAG, "Got NAK for class %02X message %02X", class_id, msg_id);
                    return GNSS_RESPONSE_NAK; // NAK received
                }
                ack = 0; // Reset the acknowledgement counter
            }
        }
    }
#ifdef GPS_DEBUG
    TT_LOG_I(TAG, "%s", debugmsg.c_str());
    TT_LOG_W(TAG, "No response for class %02X message %02X", class_id, msg_id);
#endif
    return GNSS_RESPONSE_NONE; // No response received within timeout
}

static int getAck(uart_port_t port, uint8_t* buffer, uint16_t size, uint8_t requestedClass, uint8_t requestedId, TickType_t timeout) {
    uint16_t ubxFrameCounter = 0;
    uint32_t startTime = kernel::getTicks();
    uint16_t needRead = 0;

    while (kernel::getTicks() - startTime < timeout) {
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
                case 5: {
                    // Payload length msb
                    needRead |= (c << 8);
                    ubxFrameCounter++;
                    // Check for buffer overflow
                    if (needRead >= size) {
                        ubxFrameCounter = 0;
                        break;
                    }
                    auto read_bytes = uart::readBytes(port, buffer, needRead, 250 / portTICK_PERIOD_MS);
                    if (read_bytes != needRead) {
                        ubxFrameCounter = 0;
                    } else {
                        // return payload length
#ifdef GPS_DEBUG
                        TT_LOG_I(TAG, "Got ACK for class %02X message %02X in %lums", requestedClass, requestedId, kernel::getMillis() - startTime);
#endif
                        return needRead;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
    return 0;
}

#define DETECTED_MESSAGE "%s detected, using %s Module"

static struct uBloxGnssModelInfo {
    char swVersion[30];
    char hwVersion[10];
    uint8_t extensionNo;
    char extension[10][30];
    uint8_t protocol_version;
} ublox_info;

GpsModel probe(uart_port_t port) {
    TT_LOG_I(TAG, "Probing for U-blox");

    uint8_t cfg_rate[] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00};
    checksum(cfg_rate, sizeof(cfg_rate));
    uart::flushInput(port);
    uart::writeBytes(port, cfg_rate, sizeof(cfg_rate));
    // Check that the returned response class and message ID are correct
    GPS_RESPONSE response = getAck(port, 0x06, 0x08, 750);
    if (response == GNSS_RESPONSE_NONE) {
        TT_LOG_W(TAG, "No GNSS Module (baudrate %lu)", uart::getBaudRate(port));
        return GpsModel::UNKNOWN;
    } else if (response == GNSS_RESPONSE_FRAME_ERRORS) {
        TT_LOG_W(TAG, "UBlox Frame Errors (baudrate %lu)", uart::getBaudRate(port));
    }

    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    uint8_t _message_MONVER[8] = {
        0xB5, 0x62, // Sync message for UBX protocol
        0x0A, 0x04, // Message class and ID (UBX-MON-VER)
        0x00, 0x00, // Length of payload (we're asking for an answer, so no payload)
        0x00, 0x00 // Checksum
    };
    //  Get Ublox gnss module hardware and software info
    checksum(_message_MONVER, sizeof(_message_MONVER));
    uart::flushInput(port);
    uart::writeBytes(port, _message_MONVER, sizeof(_message_MONVER));

    uint16_t len = getAck(port, buffer, sizeof(buffer), 0x0A, 0x04, 1200);
    if (len) {
        uint16_t position = 0;
        for (char& i: ublox_info.swVersion) {
            i = buffer[position];
            position++;
        }
        for (char& i: ublox_info.hwVersion) {
            i = buffer[position];
            position++;
        }

        while (len >= position + 30) {
            for (int i = 0; i < 30; i++) {
                ublox_info.extension[ublox_info.extensionNo][i] = buffer[position];
                position++;
            }
            ublox_info.extensionNo++;
            if (ublox_info.extensionNo > 9)
                break;
        }

        TT_LOG_I(TAG, "Module Info : ");
        TT_LOG_I(TAG, "Soft version: %s", ublox_info.swVersion);
        TT_LOG_I(TAG, "Hard version: %s", ublox_info.hwVersion);
        TT_LOG_I(TAG, "Extensions:%d", ublox_info.extensionNo);
        for (int i = 0; i < ublox_info.extensionNo; i++) {
            TT_LOG_I(TAG, "  %s", ublox_info.extension[i]);
        }

        memset(buffer, 0, sizeof(buffer));

        // tips: extensionNo field is 0 on some 6M GNSS modules
        for (int i = 0; i < ublox_info.extensionNo; ++i) {
            if (!strncmp(ublox_info.extension[i], "MOD=", 4)) {
                strncpy((char*)buffer, &(ublox_info.extension[i][4]), sizeof(buffer));
            } else if (!strncmp(ublox_info.extension[i], "PROTVER", 7)) {
                char* ptr = nullptr;
                memset(buffer, 0, sizeof(buffer));
                strncpy((char*)buffer, &(ublox_info.extension[i][8]), sizeof(buffer));
                TT_LOG_I(TAG, "Protocol Version:%s", (char*)buffer);
                if (strlen((char*)buffer)) {
                    ublox_info.protocol_version = strtoul((char*)buffer, &ptr, 10);
                    TT_LOG_I(TAG, "ProtVer=%d", ublox_info.protocol_version);
                } else {
                    ublox_info.protocol_version = 0;
                }
            }
        }
        if (strncmp(ublox_info.hwVersion, "00040007", 8) == 0) {
            TT_LOG_I(TAG, DETECTED_MESSAGE, "U-blox 6", "6");
            return GpsModel::UBLOX6;
        } else if (strncmp(ublox_info.hwVersion, "00070000", 8) == 0) {
            TT_LOG_I(TAG, DETECTED_MESSAGE, "U-blox 7", "7");
            return GpsModel::UBLOX7;
        } else if (strncmp(ublox_info.hwVersion, "00080000", 8) == 0) {
            TT_LOG_I(TAG, DETECTED_MESSAGE, "U-blox 8", "8");
            return GpsModel::UBLOX8;
        } else if (strncmp(ublox_info.hwVersion, "00190000", 8) == 0) {
            TT_LOG_I(TAG, DETECTED_MESSAGE, "U-blox 9", "9");
            return GpsModel::UBLOX9;
        } else if (strncmp(ublox_info.hwVersion, "000A0000", 8) == 0) {
            TT_LOG_I(TAG, DETECTED_MESSAGE, "U-blox 10", "10");
            return GpsModel::UBLOX10;
        }
    }

    return GpsModel::UNKNOWN;
}

bool init(uart_port_t port, GpsModel type) {
    TT_LOG_I(TAG, "U-blox init");
    switch (type) {
        case GpsModel::UBLOX6:
            return initUblox6(port);
        case GpsModel::UBLOX7:
        case GpsModel::UBLOX8:
        case GpsModel::UBLOX9:
            return initUblox789(port);
        case GpsModel::UBLOX10:
            return initUblox10(port);
        default:
            TT_LOG_E(TAG, "Unknown or unsupported U-blox model");
            return false;
    }
}

bool initUblox10(uart_port_t port) {
    uint8_t buffer[256];
    kernel::delayMillis(1000);
    uart::flushInput(port);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_NMEA_RAM, "disable NMEA messages in M10 RAM", 300);
    kernel::delayMillis(750);
    uart::flushInput(port);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_NMEA_BBR, "disable NMEA messages in M10 BBR", 300);
    kernel::delayMillis(750);
    uart::flushInput(port);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_TXT_INFO_RAM, "disable Info messages for M10 GPS RAM", 300);
    kernel::delayMillis(750);
    // Next disable Info txt messages in BBR layer
    uart::flushInput(port);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_TXT_INFO_BBR, "disable Info messages for M10 GPS BBR", 300);
    kernel::delayMillis(750);
    // Do M10 configuration for Power Management.
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_PM_RAM, "enable powersave for M10 GPS RAM", 300);
    kernel::delayMillis(750);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_PM_BBR, "enable powersave for M10 GPS BBR", 300);
    kernel::delayMillis(750);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_ITFM_RAM, "enable jam detection M10 GPS RAM", 300);
    kernel::delayMillis(750);
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_ITFM_BBR, "enable jam detection M10 GPS BBR", 300);
    kernel::delayMillis(750);
    // Here is where the init commands should go to do further M10 initialization.
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_SBAS_RAM, "disable SBAS M10 GPS RAM", 300);
    kernel::delayMillis(750); // will cause a receiver restart so wait a bit
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_DISABLE_SBAS_BBR, "disable SBAS M10 GPS BBR", 300);
    kernel::delayMillis(750); // will cause a receiver restart so wait a bit

    // Done with initialization, Now enable wanted NMEA messages in BBR layer so they will survive a periodic
    // sleep.
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_ENABLE_NMEA_BBR, "enable messages for M10 GPS BBR", 300);
    kernel::delayMillis(750);
    // Next enable wanted NMEA messages in RAM layer
    SEND_UBX_PACKET(port, buffer, 0x06, 0x8A, _message_VALSET_ENABLE_NMEA_RAM, "enable messages for M10 GPS RAM", 500);
    kernel::delayMillis(750);

    // As the M10 has no flash, the best we can do to preserve the config is to set it in RAM and BBR.
    // BBR will survive a restart, and power off for a while, but modules with small backup
    // batteries or super caps will not retain the config for a long power off time.
    auto msglen = makePacket(0x06, 0x09, _message_SAVE_10, sizeof(_message_SAVE_10), buffer);
    uart::writeBytes(port, buffer, msglen);
    if (getAck(port, 0x06, 0x09, 2000) != GNSS_RESPONSE_OK) {
        TT_LOG_W(TAG, "Unable to save GNSS module config");
    } else {
        TT_LOG_I(TAG, "GNSS module configuration saved!");
    }
    return true;
}

bool initUblox789(uart_port_t port) {
//    if (gnssModel == GNSS_MODEL_UBLOX7) {
//        LOG_DEBUG("Set GPS+SBAS");
//        msglen = makeUBXPacket(0x06, 0x3e, sizeof(_message_GNSS_7), _message_GNSS_7);
//        _serial_gps->write(UBXscratch, msglen);
//    } else { // 8,9
//        msglen = makeUBXPacket(0x06, 0x3e, sizeof(_message_GNSS_8), _message_GNSS_8);
//        _serial_gps->write(UBXscratch, msglen);
//    }
//
//    if (getACK(0x06, 0x3e, 800) == GNSS_RESPONSE_NAK) {
//        // It's not critical if the module doesn't acknowledge this configuration.
//        LOG_DEBUG("reconfigure GNSS - defaults maintained. Is this module GPS-only?");
//    } else {
//        if (gnssModel == GNSS_MODEL_UBLOX7) {
//            LOG_INFO("GPS+SBAS configured");
//        } else { // 8,9
//            LOG_INFO("GPS+SBAS+GLONASS+Galileo configured");
//        }
//        // Documentation say, we need wait atleast 0.5s after reconfiguration of GNSS module, before sending next
//        // commands for the M8 it tends to be more... 1 sec should be enough ;>)
//        kernel::delayMillis(1000);
//    }
//
//    // Disable Text Info messages //6,7,8,9
//    uart::flushInput(port);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x02, _message_DISABLE_TXT_INFO, "disable text info messages", 500);
//
//    if (gnssModel == GNSS_MODEL_UBLOX8) { // 8
//        uart::flushInput(port);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x39, _message_JAM_8, "enable interference resistance", 500);
//
//        uart::flushInput(port);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x23, _message_NAVX5_8, "configure NAVX5_8 settings", 500);
//    } else { // 6,7,9
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x39, _message_JAM_6_7, "enable interference resistance", 500);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x23, _message_NAVX5, "configure NAVX5 settings", 500);
//    }
//    // Turn off unwanted NMEA messages, set update rate
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x08, _message_1HZ, "set GPS update rate", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GLL, "disable NMEA GLL", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GSA, "enable NMEA GSA", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GSV, "disable NMEA GSV", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_VTG, "disable NMEA VTG", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_RMC, "enable NMEA RMC", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GGA, "enable NMEA GGA", 500);
//
//    if (ublox_info.protocol_version >= 18) {
//        uart::flushInput(port);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x86, _message_PMS, "enable powersave for GPS", 500);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x3B, _message_CFG_PM2, "enable powersave details for GPS", 500);
//
//        // For M8 we want to enable NMEA vserion 4.10 so we can see the additional sats.
//        if (gnssModel == GNSS_MODEL_UBLOX8) {
//            uart::flushInput(port);
//            SEND_UBX_PACKET(port, buffer, 0x06, 0x17, _message_NMEA, "enable NMEA 4.10", 500);
//        }
//    } else {
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x11, _message_CFG_RXM_PSM, "enable powersave mode for GPS", 500);
//        SEND_UBX_PACKET(port, buffer, 0x06, 0x3B, _message_CFG_PM2, "enable powersave details for GPS", 500);
//    }
//
//    msglen = makeUBXPacket(0x06, 0x09, sizeof(_message_SAVE), _message_SAVE);
//    _serial_gps->write(UBXscratch, msglen);
//    if (getACK(0x06, 0x09, 2000) != GNSS_RESPONSE_OK) {
//        LOG_WARN("Unable to save GNSS module config");
//    } else {
//        LOG_INFO("GNSS module configuration saved!");
//    }
    return true;
}

bool initUblox6(uart_port_t port) {
//    uart::flushInput(port);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x02, _message_DISABLE_TXT_INFO, "disable text info messages", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x39, _message_JAM_6_7, "enable interference resistance", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x23, _message_NAVX5, "configure NAVX5 settings", 500);
//
//    // Turn off unwanted NMEA messages, set update rate
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x08, _message_1HZ, "set GPS update rate", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GLL, "disable NMEA GLL", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GSA, "enable NMEA GSA", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GSV, "disable NMEA GSV", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_VTG, "disable NMEA VTG", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_RMC, "enable NMEA RMC", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_GGA, "enable NMEA GGA", 500);
//
//    uart::flushInput(port);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x11, _message_CFG_RXM_ECO, "enable powersave ECO mode for Neo-6", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x3B, _message_CFG_PM2, "enable powersave details for GPS", 500);
//    SEND_UBX_PACKET(port, buffer, 0x06, 0x01, _message_AID, "disable UBX-AID", 500);
//
//    msglen = makeUBXPacket(0x06, 0x09, sizeof(_message_SAVE), _message_SAVE);
//    _serial_gps->write(UBXscratch, msglen);
//    if (getACK(0x06, 0x09, 2000) != GNSS_RESPONSE_OK) {
//        LOG_WARN("Unable to save GNSS module config");
//    } else {
//        LOG_INFO("GNSS module config saved!");
//    }
    return true;
}

} // namespace tt::hal::gps::ublox
