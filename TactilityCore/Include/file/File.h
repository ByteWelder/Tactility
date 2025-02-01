#pragma once

#include "TactilityCore.h"
#include <cstdio>

namespace tt::file {

long getSize(FILE* file);

/** Read a file and return its data.
 * @param[in] filepath the path of the file
 * @param[out] size the amount of bytes that were read
 * @return null on error, or an array of bytes in case of success
 */
std::unique_ptr<uint8_t[]> readBinary(const std::string& filepath, size_t& outSize);

/** Read a file and return a null-terminated string that represents its content.
 * @param[in] filepath the path of the file
 * @return null on error, or an array of bytes in case of success. Empty string returns as a single 0 character.
 */
std::unique_ptr<uint8_t[]> readString(const std::string& filepath);

}
