#include "hash.h"

uint32_t tt_hash_string_djb2(const char* str) {
    uint32_t hash = 5381;
    char c = (char)*str++;
    while (c != 0) {
        hash = ((hash << 5) + hash) + (uint32_t)c; // hash * 33 + c
        c = (char)*str++;
    }
    return hash;
}
