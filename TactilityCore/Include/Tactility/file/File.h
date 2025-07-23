#pragma once

#include "Tactility/TactilityCore.h"

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

namespace tt::file {

/** File types for `dirent`'s `d_type`. */
enum {
    TT_DT_UNKNOWN = 0,
#define TT_DT_UNKNOWN TT_DT_UNKNOWN // Unknown type
    TT_DT_FIFO = 1,
#define TT_DT_FIFO TT_DT_FIFO // Named pipe or FIFO
    TT_DT_CHR = 2,
#define TT_DT_CHR TT_DT_CHR // Character device
    TT_DT_DIR = 4,
#define TT_DT_DIR TT_DT_DIR // Directory
    TT_DT_BLK = 6,
#define TT_DT_BLK TT_DT_BLK // Block device
    TT_DT_REG = 8,
#define TT_DT_REG TT_DT_REG // Regular file
    TT_DT_LNK = 10,
#define TT_DT_LNK TT_DT_LNK // Symbolic link
    TT_DT_SOCK = 12,
#define TT_DT_SOCK TT_DT_SOCK // Local-domain socket
    TT_DT_WHT = 14
#define TT_DT_WHT TT_DT_WHT // Whiteout inodes
};

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

/** Write text to a file
 * @param[in] path file path to write to
 * @param[in] content file content to write
 * @return true when operation is successful
 */
bool writeString(const std::string& filepath, const std::string& content);

/** Ensure a directory path exists.
 * @param[in] path the directory path to find, or to create recursively
 * @param[in] mode the mode to use when creating directories
 * @return true when the specified path was found, or otherwise creates the directories recursively with the specified mode.
 */
bool findOrCreateDirectory(std::string path, mode_t mode);

/**
 * Concatenate a child path with a parent path, ensuring proper slash inbetween
 * @param basePath an absolute path with or without trailing "/"
 * @param childPath the name of the child path (e.g. subfolder or file)
 * @return the concatenated path
 */
std::string getChildPath(const std::string& basePath, const std::string& childPath);

typedef int (*ScandirFilter)(const dirent*);

typedef bool (*ScandirSort)(const dirent&, const dirent&);

/** Used for sorting by alphanumeric value and file type */
bool direntSortAlphaAndType(const dirent& left, const dirent& right);

/** A filter for filtering out "." and ".." */
int direntFilterDotEntries(const dirent* entry);

/**
 * A scandir()-like implementation that works on ESP32.
 * It does not return "." and ".." items but otherwise functions the same.
 * It returns an allocated output array with allocated dirent instances.
 * The caller is responsible for free-ing the memory of these.
 *
 * @param[in] path path the scan for files and directories
 * @param[out] outList a pointer to vector of dirent
 * @param[in] filter an optional filter to filter out specific items
 * @param[in] sort an optional sorting function
 * @return the amount of items that were stored in "output" or -1 when an error occurred
 */
int scandir(
    const std::string& path,
    std::vector<dirent>& outList,
    ScandirFilter _Nullable filter,
    ScandirSort _Nullable sort
);

}
