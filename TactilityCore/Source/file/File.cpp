#include "File.h"

namespace tt::file {

#define TAG "file"

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
            data = nullptr;
            break;
        }
    }

    outSize = buffer_offset;

    fclose(file);
    return data;
}

std::unique_ptr<uint8_t[]> readBinary(const std::string& filepath, size_t& outSize) {
    return readBinaryInternal(filepath, outSize);
}

std::unique_ptr<uint8_t[]> readString(const std::string& filepath) {
    size_t size = 0;
    auto data = readBinaryInternal(filepath, size, 1);
    if (data == nullptr) {
        return nullptr;
    } else if (size > 0) {
        data.get()[size] = 0; // Append null terminator
        return data;
    } else { // Empty file: return empty string
        auto value = std::make_unique<uint8_t[]>(1);
        value[0] = 0;
        return value;
    }
}

}
