#pragma once

#if TT_FEATURE_MESH

#include <mesh/mesh.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

/** A macro, because the manifest's id is a char array that needs a string literal */
#define CONVERSATION_APP_ID "tactility.meshconversation"
#define ADD_CONTACT_APP_ID "tactility.meshaddcontact"

namespace tt::app::meshchat {

/** Arguments of the conversation app: [0] kind, [1] contact address in hex or channel index, [2] title */
constexpr auto* KIND_CONTACT = "contact";
constexpr auto* KIND_CHANNEL = "channel";

inline ::std::string addressToHex(const MeshAddress& address) {
    ::std::string text;
    char hex[3];
    for (size_t i = 0; i < address.length; i++) {
        snprintf(hex, sizeof(hex), "%02x", address.bytes[i]);
        text += hex;
    }
    return text;
}

inline bool hexToAddress(const char* hex, MeshAddress* address) {
    const size_t length = strlen(hex);
    if (length == 0 || length % 2 != 0 || length / 2 > MESH_ADDRESS_MAX_LENGTH) {
        return false;
    }
    *address = {};
    for (size_t i = 0; i < length / 2; i++) {
        char pair[3] = { hex[i * 2], hex[i * 2 + 1], '\0' };
        char* end = nullptr;
        address->bytes[i] = (uint8_t)strtoul(pair, &end, 16);
        if (end != pair + 2) {
            return false;
        }
    }
    address->length = (uint8_t)(length / 2);
    return true;
}

}

#endif
