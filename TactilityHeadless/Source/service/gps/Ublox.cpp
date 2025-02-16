#include "Tactility/service/gps/Ublox.h"

namespace tt::service::gps::ublox {

void checksum(uint8_t* message, size_t length) {
    uint8_t CK_A = 0U, CK_B = 0U;

    // Calculate the checksum, starting from the CLASS field (which is message[2])
    for (size_t i = 2U; i < length - 2U; i++) {
        CK_A = (CK_A + message[i]) & 0xFFU;
        CK_B = (CK_B + CK_A) & 0xFFU;
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

} // namespace tt::service::gps::ublox
