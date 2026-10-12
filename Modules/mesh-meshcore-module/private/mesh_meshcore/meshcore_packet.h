// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>

// Header byte: route type (bits 0-1), payload type (bits 2-5), payload version (bits 6-7)
enum class MeshCoreRoute : uint8_t {
    TransportFlood = 0,
    Flood = 1,
    Direct = 2,
    TransportDirect = 3,
};

enum class MeshCorePayload : uint8_t {
    Request = 0x00,
    Response = 0x01,
    TextMessage = 0x02,
    Ack = 0x03,
    Advert = 0x04,
    GroupText = 0x05,
    GroupData = 0x06,
    AnonymousRequest = 0x07,
    Path = 0x08,
    Trace = 0x09,
    Multipart = 0x0A,
    RawCustom = 0x0F,
};

constexpr size_t MESHCORE_MAX_PATH_SIZE = 64;
constexpr size_t MESHCORE_MAX_PAYLOAD_SIZE = 184;

struct MeshCorePacket {
    MeshCoreRoute route = MeshCoreRoute::Flood;
    MeshCorePayload payloadType = MeshCorePayload::RawCustom;
    uint8_t payloadVersion = 0;
    uint16_t transportCodes[2] = {};
    /** Number of hops in the path */
    uint8_t pathLength = 0;
    /** Bytes per hop in the path */
    uint8_t pathHashSize = 1;
    uint8_t path[MESHCORE_MAX_PATH_SIZE] = {};
    uint8_t payloadLength = 0;
    uint8_t payload[MESHCORE_MAX_PAYLOAD_SIZE] = {};

    bool hasTransportCodes() const {
        return route == MeshCoreRoute::TransportFlood || route == MeshCoreRoute::TransportDirect;
    }
};

/** @return false when the frame is malformed */
bool meshcore_packet_parse(const uint8_t* data, size_t length, MeshCorePacket* packet);

/** @return the number of bytes written, 0 when @a capacity is too small */
size_t meshcore_packet_serialize(const MeshCorePacket& packet, uint8_t* data, size_t capacity);

/** Identifies a packet regardless of the path it took, for duplicate detection */
uint32_t meshcore_packet_hash(const MeshCorePacket& packet);
