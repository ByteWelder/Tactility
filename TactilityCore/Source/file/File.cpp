#include "Tactility/file/File.h"

#include <cstring>
#include <fstream>

namespace tt::file {

#define TAG "file"

std::string getChildPath(const std::string& basePath, const std::string& childPath) {
    // Postfix with "/" when the current path isn't "/"
    if (basePath.length() != 1) {
        return basePath + "/" + childPath;
    } else {
        return "/" + childPath;
    }
}

int direntFilterDotEntries(const dirent* entry) {
    return (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) ? -1 : 0;
}

bool direntSortAlphaAndType(const dirent& left, const dirent& right) {
    bool left_is_dir = left.d_type == TT_DT_DIR || left.d_type == TT_DT_CHR;
    bool right_is_dir = right.d_type == TT_DT_DIR || right.d_type == TT_DT_CHR;
    if (left_is_dir == right_is_dir) {
        return strcmp(left.d_name, right.d_name) < 0;
    } else {
        return left_is_dir > right_is_dir;
    }
}

int scandir(
    const std::string& path,
    std::vector<dirent>& outList,
    ScandirFilter _Nullable filterMethod,
    ScandirSort _Nullable sortMethod
) {
    TT_LOG_I(TAG, "scandir start");
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        TT_LOG_E(TAG, "Failed to open dir %s", path.c_str());
        return -1;
    }

    dirent* current_entry;
    while ((current_entry = readdir(dir)) != nullptr) {
        if (filterMethod(current_entry) == 0) {
            outList.push_back(*current_entry);
        }
    }

    closedir(dir);

    if (sortMethod != nullptr) {
        std::ranges::sort(outList, sortMethod);
    }

    TT_LOG_I(TAG, "scandir finish");
    return outList.size();
}


long getSize(FILE* file) {
    long original_offset = ftell(file);

    if (fseek(file, 0, SEEK_END) != 0) {
        TT_LOG_E(TAG, "fseek failed");
        return -1;
    }

    long file_size = ftell(file);
    if (file_size == -1) {
        TT_LOG_E(TAG, "Could not get file length");
        return -1;
    }

    if (fseek(file, original_offset, SEEK_SET) != 0) {
        TT_LOG_E(TAG, "fseek Failed");
        return -1;
    }

    return file_size;
}

/** Read a file.
 * @param[in] filepath
 * @param[out] outSize the amount of bytes that were read, excluding the sizePadding
 * @param[in] sizePadding optional padding to add at the end of the output data (the values are not set)
 */
static std::unique_ptr<uint8_t[]> readBinaryInternal(const std::string& filepath, size_t& outSize, size_t sizePadding = 0) {
    FILE* file = fopen(filepath.c_str(), "rb");

    if (file == nullptr) {
        TT_LOG_E(TAG, "Failed to open %s", filepath.c_str());
        return nullptr;
    }

    long content_length = getSize(file);
    if (content_length == -1) {
        TT_LOG_E(TAG, "Failed to determine content length for %s", filepath.c_str());
        return nullptr;
    }

    auto data = std::make_unique<uint8_t[]>(content_length + sizePadding);
    if (data == nullptr) {
        TT_LOG_E(TAG, "Insufficient memory. Failed to allocate %ldl bytes.", content_length);
        return nullptr;
    }

    size_t buffer_offset = 0;
    while (buffer_offset < content_length) {
        size_t bytes_read = fread(&data.get()[buffer_offset], 1, content_length - buffer_offset, file);
        TT_LOG_D(TAG, "Read %d bytes", bytes_read);
        if (bytes_read > 0) {
            buffer_offset += bytes_read;
        } else { // Something went wrong?
            break;
        }
    }

    fclose(file);

    if (buffer_offset == content_length) {
        outSize = buffer_offset;
        return data;
    } else {
        outSize = 0;
        return nullptr;
    }
}

std::unique_ptr<uint8_t[]> readBinary(const std::string& filepath, size_t& outSize) {
    return readBinaryInternal(filepath, outSize);
}

std::unique_ptr<uint8_t[]> readString(const std::string& filepath) {
    size_t size = 0;
    auto data = readBinaryInternal(filepath, size, 1);
    if (data == nullptr) {
        return nullptr;
    }

    data.get()[size] = 0; // Append null terminator
    return data;
}

bool writeString(const std::string& filepath, const std::string& content) {
    std::ofstream fileStream(filepath);

    if (!fileStream.is_open()) {
        return false;
    }

    fileStream << content;
    fileStream.close();

    return true;
}

static bool findOrCreateDirectoryInternal(std::string path, mode_t mode) {
    struct stat dir_stat;
    if (mkdir(path.c_str(), mode) == 0) {
        return true;
    }

    if (errno != EEXIST) {
        return false;
    }

    if (stat(path.c_str(), &dir_stat) != 0) {
        return false;
    }

    if (!S_ISDIR(dir_stat.st_mode)) {
        return false;
    }

    return true;
}

bool findOrCreateDirectory(std::string path, mode_t mode) {
    if (path.empty()) {
        return true;
    }
    TT_LOG_D(TAG, "findOrCreate: %s %lu", path.c_str(), mode);

    const char separator_to_find[] = {SEPARATOR, 0x00};
    auto first_index = path[0] == SEPARATOR ? 1 : 0;
    auto separator_index = path.find(separator_to_find, first_index);

    bool should_break = false;
    while (!should_break) {
        bool is_last_segment = (separator_index == std::string::npos);
        auto to_create = is_last_segment ? path : path.substr(0, separator_index);
        should_break = is_last_segment;
        if (!findOrCreateDirectoryInternal(to_create, mode)) {
            TT_LOG_E(TAG, "Failed to create %s", to_create.c_str());
            return false;
        } else {
            TT_LOG_D(TAG, "  - got: %s", to_create.c_str());
        }

        // Find next file separator index
        separator_index = path.find(separator_to_find, separator_index + 1);
    }

    return true;
}

}
