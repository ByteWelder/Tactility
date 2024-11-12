#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is quicker than the m-string.h hashing, as the latter
 * operates on raw memory blocks and thus a strlen() call is required first.
 * @param[in] str the string to calculate the hash for
 * @return the hash
 */
uint32_t tt_hash_string_djb2(const char* str);

/**
 * This is quicker than the m-string.h hashing, as the latter
 * operates on raw memory blocks and thus a strlen() call is required first.
 * @param[in] data the bytes to calculate the hash for
 * @return the hash
 */
uint32_t tt_hash_bytes_djb2(const void* data, size_t length);

#ifdef __cplusplus
}
#endif