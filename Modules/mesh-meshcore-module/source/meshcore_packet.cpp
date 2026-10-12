// SPDX-License-Identifier: Apache-2.0
#include <mesh_meshcore/meshcore_crypto.h>
#include <mesh_meshcore/meshcore_packet.h>

#include <cstring>

bool meshcore_packet_parse(const uint8_t* data, size_t length, MeshCorePacket* packet) {
    size_t position = 0;
    if (length < 2) {
        return false;
    }

    const uint8_t header = data[position++];
    packet->route = static_cast<MeshCoreRoute>(header & 0x03);
    packet->payloadType = static_cast<MeshCorePayload>((header >> 2) & 0x0F);
    packet->payloadVersion = (header >> 6) & 0x03;

    if (packet->hasTransportCodes()) {
        if (position + 4 > length) {
            return false;
        }
        packet->transportCodes[0] = data[position] | (data[position + 1] << 8);
        packet->transportCodes[1] = data[position + 2] | (data[position + 3] << 8);
        position += 4;
    }

    if (position >= length) {
        return false;
    }
    // The upper 2 bits hold the hash size minus one, the lower 6 bits the hop count
    const uint8_t pathByte = data[position++];
    packet->pathLength = pathByte & 0x3F;
    packet->pathHashSize = (pathByte >> 6) + 1;
    const size_t pathBytes = (size_t)packet->pathLength * packet->pathHashSize;
    if (pathBytes > MESHCORE_MAX_PATH_SIZE || position + pathBytes > length) {
        return false;
    }
    memcpy(packet->path, data + position, pathBytes);
    position += pathBytes;

    const size_t payloadLength = length - position;
    if (payloadLength > MESHCORE_MAX_PAYLOAD_SIZE) {
        return false;
    }
    packet->payloadLength = (uint8_t)payloadLength;
    memcpy(packet->payload, data + position, payloadLength);
    return true;
}

size_t meshcore_packet_serialize(const MeshCorePacket& packet, uint8_t* data, size_t capacity) {
    const size_t pathBytes = (size_t)packet.pathLength * packet.pathHashSize;
    const size_t needed = 1 + (packet.hasTransportCodes() ? 4 : 0) + 1 + pathBytes + packet.payloadLength;
    if (needed > capacity || pathBytes > MESHCORE_MAX_PATH_SIZE || packet.pathHashSize == 0 || packet.pathHashSize > 4) {
        return 0;
    }

    size_t position = 0;
    data[position++] = (uint8_t)(((uint8_t)packet.route & 0x03) | (((uint8_t)packet.payloadType & 0x0F) << 2) | ((packet.payloadVersion & 0x03) << 6));
    if (packet.hasTransportCodes()) {
        data[position++] = (uint8_t)packet.transportCodes[0];
        data[position++] = (uint8_t)(packet.transportCodes[0] >> 8);
        data[position++] = (uint8_t)packet.transportCodes[1];
        data[position++] = (uint8_t)(packet.transportCodes[1] >> 8);
    }
    data[position++] = (uint8_t)((packet.pathLength & 0x3F) | ((packet.pathHashSize - 1) << 6));
    memcpy(data + position, packet.path, pathBytes);
    position += pathBytes;
    memcpy(data + position, packet.payload, packet.payloadLength);
    position += packet.payloadLength;
    return position;
}

uint32_t meshcore_packet_hash(const MeshCorePacket& packet) {
    const uint8_t type = (uint8_t)packet.payloadType;
    uint8_t hash[4];
    meshcore_sha256_pair(hash, sizeof(hash), &type, 1, packet.payload, packet.payloadLength);
    return hash[0] | (hash[1] << 8) | (hash[2] << 16) | ((uint32_t)hash[3] << 24);
}
