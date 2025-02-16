#include "Tactility/service/gps/Ublox.h"

namespace tt::service::gps::ublox {

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

} // namespace tt::service::gps::ublox
