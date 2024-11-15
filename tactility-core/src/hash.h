#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Implementation of DJB2 hashing algorithm.
 * @param[in] str the string to calculate the hash for
 * @return the hash
 */
uint32_t tt_hash_string_djb2(const char* str);

/**
 * Implementation of DJB2 hashing algorithm.
 * @param[in] data the bytes to calculate the hash for
 * @return the hash
 */
uint32_t tt_hash_bytes_djb2(const void* data, size_t length);
