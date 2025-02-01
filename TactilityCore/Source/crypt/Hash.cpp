#include "Tactility/crypt/Hash.h"

namespace tt::crypt {

uint32_t djb2(const char* str) {
    uint32_t hash = 5381;
    char c = (char)*str++;
    while (c != 0) {
        hash = ((hash << 5) + hash) + (uint32_t)c; // hash * 33 + c
        c = (char)*str++;
    }
    return hash;
}

uint32_t djb2(const void* data, size_t length) {
    uint32_t hash = 5381;
    auto* data_bytes = static_cast<const uint8_t*>(data);
    uint8_t c = *data_bytes++;
    size_t index = 0;
    while (index < length) {
        hash = ((hash << 5) + hash) + (uint32_t)c; // hash * 33 + c
        c = *data_bytes++;
        index++;
    }
    return hash;
}

} // namespace
