#pragma once

#include "TactilityCore.h"
#include <cstdio>

namespace tt::file {

long getSize(FILE* file);

std::unique_ptr<uint8_t[]> readBinary(const std::string& filepath, size_t& outSize);
std::unique_ptr<uint8_t[]> readString(const std::string& filepath);

}
