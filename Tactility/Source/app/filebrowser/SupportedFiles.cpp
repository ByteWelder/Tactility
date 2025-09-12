#include <Tactility/StringUtils.h>
#include <Tactility/TactilityCore.h>

namespace tt::app::filebrowser {

constexpr auto* TAG = "FileBrowser";

bool isSupportedAppFile(const std::string& filename) {
    return filename.ends_with(".app");
}

bool isSupportedImageFile(const std::string& filename) {
    // Currently only the PNG library is built into Tactility
    return string::lowercase(filename).ends_with(".png");
}

bool isSupportedTextFile(const std::string& filename) {
    std::string filename_lower = string::lowercase(filename);
    return filename_lower.ends_with(".txt") ||
        filename_lower.ends_with(".ini") ||
        filename_lower.ends_with(".json") ||
        filename_lower.ends_with(".yaml") ||
        filename_lower.ends_with(".yml") ||
        filename_lower.ends_with(".lua") ||
        filename_lower.ends_with(".js") ||
        filename_lower.ends_with(".properties");
}

} // namespace tt::app::filebrowser
