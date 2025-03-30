#pragma once

#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/uart/Uart.h"

#include <cstdint>
#include <cstddef>

namespace tt::hal::gps::ublox {

void checksum(uint8_t* message, size_t length);

// From https://github.com/meshtastic/firmware/blob/7648391f91f2b84e367ae2b38220b30936fb45b1/src/gps/GPS.cpp#L128
uint8_t makePacket(uint8_t classId, uint8_t messageId, const uint8_t* payload, uint8_t payloadSize, uint8_t* bufferOut);

template<size_t DataSize>
inline void sendPacket(uart_port_t port, uint8_t type, uint8_t id, uint8_t data[DataSize], const char* errorMessage, TickType_t timeout) {
    static uint8_t buffer[250] = {0};
    size_t length = makePacket(type, id, data, DataSize, buffer);
//    hal::uart::writeBytes(port, buffer, length);
}

GpsModel probe(uart::Uart& uart);

bool init(uart::Uart& uart, GpsModel model);

} // namespace tt::service::gps
