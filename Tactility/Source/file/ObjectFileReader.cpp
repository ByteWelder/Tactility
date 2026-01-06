#include <Tactility/file/ObjectFile.h>
#include <Tactility/file/ObjectFilePrivate.h>

#include <cstring>
#include <Tactility/Logger.h>

namespace tt::file {

static const auto LOGGER = Logger("ObjectFileReader");

bool ObjectFileReader::open() {
    auto opening_file = std::unique_ptr<FILE, FileCloser>(fopen(filePath.c_str(), "r"));
    if (opening_file == nullptr) {
        LOGGER.error("Failed to open file {}", filePath.c_str());
        return false;
    }

    FileHeader file_header;
    if (fread(&file_header, sizeof(FileHeader), 1, opening_file.get()) != 1) {
        LOGGER.error("Failed to read file header from {}", filePath);
        return false;
    }

    if (file_header.identifier != OBJECT_FILE_IDENTIFIER) {
        LOGGER.error("Invalid file type for {}", filePath);
        return false;
    }

    if (file_header.version != OBJECT_FILE_VERSION) {
        LOGGER.error("Unknown version for {}: {}", filePath, file_header.identifier);
        return false;
    }

    ContentHeader content_header;
    if (fread(&content_header, sizeof(ContentHeader), 1, opening_file.get()) != 1) {
        LOGGER.error("Failed to read content header from {}", filePath);
        return false;
    }

    if (recordSize != content_header.recordSize) {
        LOGGER.error("Record size mismatch for {}: expected {}, got {}", filePath, recordSize, content_header.recordSize);
        return false;
    }

    recordCount = content_header.recordCount;
    recordVersion = content_header.recordVersion;

    file = std::move(opening_file);

    LOGGER.debug("File version: {}", file_header.version);
    LOGGER.debug("Content: version = {}, size = {} bytes, count = {}", content_header.recordVersion, content_header.recordSize, content_header.recordCount);

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
        LOGGER.error("File not open");
        return false;
    }

    bool result = fread(output, recordSize, 1, file.get()) == 1;
    if (result) {
        recordsRead++;
    }

    return result;
}

}