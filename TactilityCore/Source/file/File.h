#pragma once

#include "TactilityCore.h"
#include <cstdio>

namespace tt::file {

#define TAG "file"

long getSize(FILE* file);

std::unique_ptr<uint8_t[]> readBinary(const char* filepath, size_t& outSize);
std::unique_ptr<uint8_t[]> readString(const char* filepath);

}
