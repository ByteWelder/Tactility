#pragma once

#include <string>

namespace tt::app::filebrowser {

bool isSupportedExecutableFile(const std::string& filename);
bool isSupportedImageFile(const std::string& filename);
bool isSupportedTextFile(const std::string& filename);

} // namespace
