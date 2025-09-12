#pragma once

#include <string>

namespace tt::app::filebrowser {

bool isSupportedAppFile(const std::string& filename);
bool isSupportedImageFile(const std::string& filename);
bool isSupportedTextFile(const std::string& filename);

} // namespace
