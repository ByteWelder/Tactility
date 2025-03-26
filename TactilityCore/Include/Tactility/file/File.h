#pragma once

#include "Tactility/TactilityCore.h"

#include <cstdio>
#include <sys/stat.h>

namespace tt::file {

#ifdef _WIN32
constexpr char SEPARATOR = '\\';
#else
constexpr char SEPARATOR = '/';
#endif

struct FileCloser {
    void operator()(FILE* file) {
        if (file != nullptr) {
            fclose(file);
        }
    }
};

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

/** Ensure a directory path exists.
 * @param[in] path the directory path to find, or to create recursively
 * @param[in] mode the mode to use when creating directories
 * @return true when the specified path was found, or otherwise creates the directories recursively with the specified mode.
 */
bool findOrCreateDirectory(std::string path, mode_t mode);

}
