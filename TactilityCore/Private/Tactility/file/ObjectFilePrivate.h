#pragma once

namespace tt::file {

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

}
