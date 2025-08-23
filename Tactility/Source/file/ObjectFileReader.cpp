#include "Tactility/file/ObjectFile.h"
#include "Tactility/file/ObjectFilePrivate.h"

#include <cstring>
#include <Tactility/Log.h>

namespace tt::file {

constexpr auto* TAG = "ObjectFileReader";

bool ObjectFileReader::open() {
    auto opening_file = std::unique_ptr<FILE, FileCloser>(fopen(filePath.c_str(), "r"));
    if (opening_file == nullptr) {
        TT_LOG_E(TAG, "Failed to open file %s", filePath.c_str());
        return false;
    }

    FileHeader file_header;
    if (fread(&file_header, sizeof(FileHeader), 1, opening_file.get()) != 1) {
        TT_LOG_E(TAG, "Failed to read file header from %s", filePath.c_str());
        return false;
    }

    if (file_header.identifier != OBJECT_FILE_IDENTIFIER) {
        TT_LOG_E(TAG, "Invalid file type for %s", filePath.c_str());
        return false;
    }

    if (file_header.version != OBJECT_FILE_VERSION) {
        TT_LOG_E(TAG, "Unknown version for %s: %lu", filePath.c_str(), file_header.identifier);
        return false;
    }

    ContentHeader content_header;
    if (fread(&content_header, sizeof(ContentHeader), 1, opening_file.get()) != 1) {
        TT_LOG_E(TAG, "Failed to read content header from %s", filePath.c_str());
        return false;
    }

    if (recordSize != content_header.recordSize) {
        TT_LOG_E(TAG, "Record size mismatch for %s: expected %lu, got %lu", filePath.c_str(), recordSize, content_header.recordSize);
        return false;
    }

    recordCount = content_header.recordCount;
    recordVersion = content_header.recordVersion;

    file = std::move(opening_file);

    TT_LOG_D(TAG, "File version: %lu", file_header.version);
    TT_LOG_D(TAG, "Content: version = %lu, size = %lu bytes, count = %lu", content_header.recordVersion, content_header.recordSize, content_header.recordCount);

    return true;
}

void ObjectFileReader::close() {
    recordCount = 0;
    recordVersion = 0;
    recordsRead = 0;

    file = nullptr;
}

bool ObjectFileReader::readNext(void* output) {
    if (file == nullptr) {
        TT_LOG_E(TAG, "File not open");
        return false;
    }

    bool result = fread(output, recordSize, 1, file.get()) == 1;
    if (result) {
        recordsRead++;
    }

    return result;
}

}