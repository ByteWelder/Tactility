#include "Tactility/hal/gps/Cas.h"
#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/gps/Ublox.h"
#include <cstring>

#define TAG "gps"

namespace tt::hal::gps {

bool initMtk(uart::Uart& uart);
bool initMtkL76b(uart::Uart& uart);
bool initMtkPa1616s(uart::Uart& uart);
bool initAtgm336h(uart::Uart& uart);
bool initUc6580(uart::Uart& uart);
bool initAg33xx(uart::Uart& uart);

// region CAS

// Calculate the checksum for a CAS packet
static void CASChecksum(uint8_t *message, size_t length)
{
    uint32_t cksum = ((uint32_t)message[5] << 24); // Message ID
    cksum += ((uint32_t)message[4]) << 16;         // Class
    cksum += message[2];                           // Payload Len

    // Iterate over the payload as a series of uint32_t's and
    // accumulate the cksum
    for (size_t i = 0; i < (length - 10) / 4; i++) {
        uint32_t pl = 0;
        memcpy(&pl, (message + 6) + (i * sizeof(uint32_t)), sizeof(uint32_t)); // avoid pointer dereference
        cksum += pl;
    }

    // Place the checksum values in the message
    message[length - 4] = (cksum & 0xFF);
    message[length - 3] = (cksum & (0xFF << 8)) >> 8;
    message[length - 2] = (cksum & (0xFF << 16)) >> 16;
    message[length - 1] = (cksum & (0xFF << 24)) >> 24;
}

// Function to create a CAS packet for editing in memory
static uint8_t makeCASPacket(uint8_t* buffer, uint8_t class_id, uint8_t msg_id, uint8_t payload_size, const uint8_t *msg)
{
    // General CAS structure
    //        | H1   | H2   | payload_len | cls  | msg  | Payload       ...   | Checksum                  |
    // Size:  | 1    | 1    | 2           | 1    | 1    | payload_len         | 4                         |
    // Pos:   | 0    | 1    | 2    | 3    | 4    | 5    | 6    | 7      ...   | 6 + payload_len ...       |
    //        |------|------|-------------|------|------|------|--------------|---------------------------|
    //        | 0xBA | 0xCE | 0xXX | 0xXX | 0xXX | 0xXX | 0xXX | 0xXX   ...   | 0xXX | 0xXX | 0xXX | 0xXX |

    // Construct the CAS packet
    buffer[0] = 0xBA;         // header 1 (0xBA)
    buffer[1] = 0xCE;         // header 2 (0xCE)
    buffer[2] = payload_size; // length 1
    buffer[3] = 0;            // length 2
    buffer[4] = class_id;     // class
    buffer[5] = msg_id;       // id

    buffer[6 + payload_size] = 0x00; // Checksum
    buffer[7 + payload_size] = 0x00;
    buffer[8 + payload_size] = 0x00;
    buffer[9 + payload_size] = 0x00;

    for (int i = 0; i < payload_size; i++) {
        buffer[6 + i] = msg[i];
    }
    CASChecksum(buffer, (payload_size + 10));

    return (payload_size + 10);
}

GpsResponse getACKCas(uart::Uart& uart, uint8_t class_id, uint8_t msg_id, uint32_t waitMillis)
{
    uint32_t startTime = kernel::getMillis();
    uint8_t buffer[CAS_ACK_NACK_MSG_SIZE] = {0};
    uint8_t bufferPos = 0;

    // CAS-ACK-(N)ACK structure
    //         | H1   | H2   | Payload Len | cls  | msg  | Payload                   | Checksum (4)              |
    //         |      |      |             |      |      | Cls  | Msg  | Reserved    |                           |
    //         |------|------|-------------|------|------|------|------|-------------|---------------------------|
    // ACK-NACK| 0xBA | 0xCE | 0x04 | 0x00 | 0x05 | 0x00 | 0xXX | 0xXX | 0x00 | 0x00 | 0xXX | 0xXX | 0xXX | 0xXX |
    // ACK-ACK | 0xBA | 0xCE | 0x04 | 0x00 | 0x05 | 0x01 | 0xXX | 0xXX | 0x00 | 0x00 | 0xXX | 0xXX | 0xXX | 0xXX |

    while (kernel::getTicks() - startTime < waitMillis) {
        if (uart.available()) {
            uart.readByte(&buffer[bufferPos++]);

            // keep looking at the first two bytes of buffer until
            // we have found the CAS frame header (0xBA, 0xCE), if not
            // keep reading bytes until we find a frame header or we run
            // out of time.
            if ((bufferPos == 2) && !(buffer[0] == 0xBA && buffer[1] == 0xCE)) {
                buffer[0] = buffer[1];
                buffer[1] = 0;
                bufferPos = 1;
            }
        }

        // we have read all the bytes required for the Ack/Nack (14-bytes)
        // and we must have found a frame to get this far
        if (bufferPos == sizeof(buffer) - 1) {
            uint8_t msg_cls = buffer[4];     // message class should be 0x05
            uint8_t msg_msg_id = buffer[5];  // message id should be 0x00 or 0x01
            uint8_t payload_cls = buffer[6]; // payload class id
            uint8_t payload_msg = buffer[7]; // payload message id

            // Check for an ACK-ACK for the specified class and message id
            if ((msg_cls == 0x05) && (msg_msg_id == 0x01) && payload_cls == class_id && payload_msg == msg_id) {
#ifdef GPS_DEBUG
                LOG_INFO("Got ACK for class %02X message %02X in %dms", class_id, msg_id, millis() - startTime);
#endif
                return GpsResponse::Ok;
            }

            // Check for an ACK-NACK for the specified class and message id
            if ((msg_cls == 0x05) && (msg_msg_id == 0x00) && payload_cls == class_id && payload_msg == msg_id) {
#ifdef GPS_DEBUG
                LOG_WARN("Got NACK for class %02X message %02X in %dms", class_id, msg_id, millis() - startTime);
#endif
                return GpsResponse::NotAck;
            }

            // This isn't the frame we are looking for, clear the buffer
            // and try again until we run out of time.
            memset(buffer, 0x0, sizeof(buffer));
            bufferPos = 0;
        }
    }
    return GpsResponse::None;
}

// endregion

bool init(uart::Uart& uart, GpsModel type) {
    switch (type) {
        case GpsModel::Unknown:
            tt_crash();
        case GpsModel::AG3335:
        case GpsModel::AG3352:
            return initAg33xx(uart);
        case GpsModel::ATGM336H:
            return initAtgm336h(uart);
        case GpsModel::LS20031:
            return true;
        case GpsModel::MTK:
            return initMtk(uart);
        case GpsModel::MTK_L76B:
            return initMtkL76b(uart);
        case GpsModel::MTK_PA1616S:
            return initMtkPa1616s(uart);
        case GpsModel::UBLOX6:
        case GpsModel::UBLOX7:
        case GpsModel::UBLOX8:
        case GpsModel::UBLOX9:
        case GpsModel::UBLOX10:
            return ublox::init(uart, type);
        case GpsModel::UC6580:
            return initUc6580(uart);
    }

    TT_LOG_I(TAG, "Init not implemented %d", static_cast<int>(type));
    return false;
}

bool initAg33xx(uart::Uart& uart) {
    uart.writeString("$PAIR066,1,0,1,0,0,1*3B\r\n"); // Enable GPS+GALILEO+NAVIC

    // Configure NMEA (sentences will output once per fix)
    uart.writeString("$PAIR062,0,1*3F\r\n"); // GGA ON
    uart.writeString("$PAIR062,1,0*3F\r\n"); // GLL OFF
    uart.writeString("$PAIR062,2,0*3C\r\n"); // GSA OFF
    uart.writeString("$PAIR062,3,0*3D\r\n"); // GSV OFF
    uart.writeString("$PAIR062,4,1*3B\r\n"); // RMC ON
    uart.writeString("$PAIR062,5,0*3B\r\n"); // VTG OFF
    uart.writeString("$PAIR062,6,0*38\r\n"); // ZDA ON

    kernel::delayMillis(250);
    uart.writeString("$PAIR513*3D\r\n"); // save configuration
    return true;
}

bool initUc6580(uart::Uart& uart) {
    // The Unicore UC6580 can use a lot of sat systems, enable it to
    // use GPS L1 & L5 + BDS B1I & B2a + GLONASS L1 + GALILEO E1 & E5a + SBAS + QZSS
    // This will reset the receiver, so wait a bit afterwards
    // The paranoid will wait for the OK*04 confirmation response after each command.
    uart.writeString("$CFGSYS,h35155\r\n");
    kernel::delayMillis(750);
    // Must be done after the CFGSYS command
    // Turn off GSV messages, we don't really care about which and where the sats are, maybe someday.
    uart.writeString("$CFGMSG,0,3,0\r\n");
    kernel::delayMillis(250);
    // Turn off GSA messages, TinyGPS++ doesn't use this message.
    uart.writeString("$CFGMSG,0,2,0\r\n");
    kernel::delayMillis(250);
    // Turn off NOTICE __TXT messages, these may provide Unicore some info but we don't care.
    uart.writeString("$CFGMSG,6,0,0\r\n");
    kernel::delayMillis(250);
    uart.writeString("$CFGMSG,6,1,0\r\n");
    kernel::delayMillis(250);
    return true;
}

bool initAtgm336h(uart::Uart& uart) {
    uint8_t buffer[256];

    // Set the intial configuration of the device - these _should_ work for most AT6558 devices
    int msglen = makeCASPacket(buffer, 0x06, 0x07, sizeof(_message_CAS_CFG_NAVX_CONF), _message_CAS_CFG_NAVX_CONF);
    uart.writeBytes(buffer, msglen);
    if (getACKCas(uart, 0x06, 0x07, 250) != GpsResponse::Ok) {
        TT_LOG_W(TAG, "ATGM336H: Could not set Config");
    }

    // Set the update frequence to 1Hz
    msglen = makeCASPacket(buffer, 0x06, 0x04, sizeof(_message_CAS_CFG_RATE_1HZ), _message_CAS_CFG_RATE_1HZ);
    uart.writeBytes(buffer, msglen);
    if (getACKCas(uart, 0x06, 0x04, 250) != GpsResponse::Ok) {
        TT_LOG_W(TAG, "ATGM336H: Could not set Update Frequency");
    }

    // Set the NEMA output messages
    // Ask for only RMC and GGA
    uint8_t fields[] = {CAS_NEMA_RMC, CAS_NEMA_GGA};
    for (unsigned int i = 0; i < sizeof(fields); i++) {
        // Construct a CAS-CFG-MSG packet
        uint8_t cas_cfg_msg_packet[] = {0x4e, fields[i], 0x01, 0x00};
        msglen = makeCASPacket(buffer, 0x06, 0x01, sizeof(cas_cfg_msg_packet), cas_cfg_msg_packet);
        uart.writeBytes(buffer, msglen);
        if (getACKCas(uart, 0x06, 0x01, 250) != GpsResponse::Ok) {
            TT_LOG_W(TAG, "ATGM336H: Could not enable NMEA MSG: %d", fields[i]);
        }
    }
    return true;
}

bool initMtkPa1616s(uart::Uart& uart) {
    // PA1616S is used in some GPS breakout boards from Adafruit
    // PA1616S does not have GLONASS capability. PA1616D does, but is not implemented here.
    uart.writeString("$PMTK353,1,0,0,0,0*2A\r\n");
    // Above command will reset the GPS and takes longer before it will accept new commands
    kernel::delayMillis(1000);
    // Only ask for RMC and GGA (GNRMC and GNGGA)
    uart.writeString("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
    kernel::delayMillis(250);
    // Enable SBAS / WAAS
    uart.writeString("$PMTK301,2*2E\r\n");
    kernel::delayMillis(250);
    return true;
}

bool initMtkL76b(uart::Uart& uart) {
    // Waveshare Pico-GPS hat uses the L76B with 9600 baud
    // Initialize the L76B Chip, use GPS + GLONASS
    // See note in L76_Series_GNSS_Protocol_Specification, chapter 3.29
    uart.writeString("$PMTK353,1,1,0,0,0*2B\r\n");
    // Above command will reset the GPS and takes longer before it will accept new commands
    kernel::delayMillis(1000);
    // only ask for RMC and GGA (GNRMC and GNGGA)
    // See note in L76_Series_GNSS_Protocol_Specification, chapter 2.1
    uart.writeString("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
    kernel::delayMillis(250);
    // Enable SBAS
    uart.writeString("$PMTK301,2*2E\r\n");
    kernel::delayMillis(250);
    // Enable PPS for 2D/3D fix only
    uart.writeString("$PMTK285,3,100*3F\r\n");
    kernel::delayMillis(250);
    // Switch to Fitness Mode, for running and walking purpose with low speed (<5 m/s)
    uart.writeString("$PMTK886,1*29\r\n");
    kernel::delayMillis(250);
    return true;
}

bool initMtk(uart::Uart& uart) {
    // Initialize the L76K Chip, use GPS + GLONASS + BEIDOU
    uart.writeString("$PCAS04,7*1E\r\n");
    kernel::delayMillis(250);
    // only ask for RMC and GGA
    uart.writeString("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    kernel::delayMillis(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    uart.writeString("$PCAS11,3*1E\r\n");
    kernel::delayMillis(250);
    return true;
}

} // namespace tt::hal::gps
