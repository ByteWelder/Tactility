#pragma once

#include <Tactility/file/File.h>

#include <string>

#include "FileLock.h"

/**
 * @warning The functionality below does NOT safely acquire file locks. Use file::getLock() or file::withLock() when using the functionality below.
 */
namespace tt::file {

class ObjectFileReader {

    const std::string filePath;
    const uint32_t recordSize = 0;

    std::unique_ptr<FILE, FileCloser> file;
    uint32_t recordCount = 0;
    uint32_t recordVersion = 0;
    uint32_t recordsRead = 0;

public:

    ObjectFileReader(std::string filePath, uint32_t recordSize) :
        filePath(std::move(filePath)),
        recordSize(recordSize)
    {}

    bool open();
    void close();

    bool hasNext() const { return recordsRead < recordCount; }
    bool readNext(void* output);

    uint32_t getRecordCount() const { return recordCount; }
    uint32_t getRecordSize() const { return recordSize; }
    uint32_t getRecordVersion() const { return recordVersion; }
};

class ObjectFileWriter {

    const std::string filePath;
    const uint32_t recordSize;
    const uint32_t recordVersion;
    const bool append;
    const std::shared_ptr<Lock> lock;

    std::unique_ptr<FILE, FileCloser> file;
    uint32_t recordsWritten = 0;

public:

    ObjectFileWriter(std::string filePath, uint32_t recordSize, uint32_t recordVersion, bool append) :
        filePath(std::move(filePath)),
        recordSize(recordSize),
        recordVersion(recordVersion),
        append(append),
        lock(getLock(filePath))
    {}


    ~ObjectFileWriter() {
        if (file != nullptr) {
            close();
        }
    }

    bool open();
    void close();

    bool write(void* data);
};

}
