#include "Tactility/file/ObjectFile.h"

#include <Tactility/Log.h>
#include <unistd.h>

namespace tt::file {

constexpr const char* TAG = "ObjectFile";

constexpr uint32_t OBJECT_FILE_IDENTIFIER = 0x13371337;
constexpr uint32_t OBJECT_FILE_VERSION = 1;

struct FileHeader {
    uint32_t identifier = OBJECT_FILE_IDENTIFIER;
    uint32_t version = OBJECT_FILE_VERSION;
};

struct ContentHeader {
    uint32_t recordVersion = 0;
    uint32_t recordSize = 0;
    uint32_t recordCount = 0;
};

// region Reader

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

// endregion Reader

// region Writer

bool ObjectFileWriter::open() {
    // If file exists
    bool edit_existing = append && access(filePath.c_str(), F_OK) == 0;

    auto* mode = edit_existing ? "r+" : "w";
    auto opening_file = std::unique_ptr<FILE, FileCloser>(fopen(filePath.c_str(), mode));
    if (opening_file == nullptr) {
        TT_LOG_E(TAG, "Failed to open file %s", filePath.c_str());
        return false;
    }

    auto file_size = getSize(opening_file.get());
    if (file_size > 0 && edit_existing) {

        // Read and parse file header

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

        // Read and parse content header

        ContentHeader content_header;
        if (fread(&content_header, sizeof(ContentHeader), 1, opening_file.get()) != 1) {
            TT_LOG_E(TAG, "Failed to read content header from %s", filePath.c_str());
            return false;
        }

        if (recordSize != content_header.recordSize) {
            TT_LOG_E(TAG, "Record size mismatch for %s: expected %lu, got %lu", filePath.c_str(), recordSize, content_header.recordSize);
            return false;
        }

        if (recordVersion != content_header.recordVersion) {
            TT_LOG_E(TAG, "Version mismatch for %s: expected %lu, got %lu", filePath.c_str(), recordVersion, content_header.recordVersion);
            return false;
        }

        recordsWritten = content_header.recordCount;
        fseek(opening_file.get(), 0, SEEK_END);
    } else {
        FileHeader file_header;
        if (fwrite(&file_header, sizeof(FileHeader), 1, opening_file.get()) != 1) {
            TT_LOG_E(TAG, "Failed to write file header for %s", filePath.c_str());
            return false;
        }

        // Seek forward (skip ContentHeader that will be written later)
        fseek(opening_file.get(), sizeof(ContentHeader), SEEK_CUR);
    }


    file = std::move(opening_file);
    return true;
}

void ObjectFileWriter::close() {
    if (file == nullptr) {
        TT_LOG_E(TAG, "File not opened: %s", filePath.c_str());
        return;
    }

    if (fseek(file.get(), sizeof(FileHeader), SEEK_SET) != 0) {
        TT_LOG_E(TAG, "File seek failed: %s", filePath.c_str());
        return;
    }

    ContentHeader content_header = {
        .recordVersion = this->recordVersion,
        .recordSize = this->recordSize,
        .recordCount = this->recordsWritten
    };

    if (fwrite(&content_header, sizeof(ContentHeader), 1, file.get()) != 1) {
        TT_LOG_E(TAG, "Failed to write content header to %s", filePath.c_str());
    }

    file = nullptr;
}

bool ObjectFileWriter::write(void* data) {
    if (file == nullptr) {
        TT_LOG_E(TAG, "File not opened: %s", filePath.c_str());
        return false;
    }

    if (fwrite(data, recordSize, 1, file.get()) != 1) {
        TT_LOG_E(TAG, "Failed to write record to %s", filePath.c_str());
        return false;
    }

    recordsWritten++;

    return true;
}

// endregion Writer

}