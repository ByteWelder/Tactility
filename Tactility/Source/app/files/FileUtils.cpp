#include "Tactility/app/files/FileUtils.h"

#include <Tactility/StringUtils.h>
#include <Tactility/TactilityCore.h>

#include <cstring>

namespace tt::app::files {

#define TAG "file_utils"

std::string getChildPath(const std::string& basePath, const std::string& childPath) {
    // Postfix with "/" when the current path isn't "/"
    if (basePath.length() != 1) {
        return basePath + "/" + childPath;
    } else {
        return "/" + childPath;
    }
}

int dirent_filter_dot_entries(const struct dirent* entry) {
    return (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) ? -1 : 0;
}

bool dirent_sort_alpha_and_type(const struct dirent& left, const struct dirent& right) {
    bool left_is_dir = left.d_type == TT_DT_DIR || left.d_type == TT_DT_CHR;
    bool right_is_dir = right.d_type == TT_DT_DIR || right.d_type == TT_DT_CHR;
    if (left_is_dir == right_is_dir) {
        return strcmp(left.d_name, right.d_name) < 0;
    } else {
        return left_is_dir > right_is_dir;
    }
}

int scandir(
    const std::string& path,
    std::vector<dirent>& outList,
    ScandirFilter _Nullable filterMethod,
    ScandirSort _Nullable sortMethod
) {
    TT_LOG_I(TAG, "scandir start");
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        TT_LOG_E(TAG, "Failed to open dir %s", path.c_str());
        return -1;
    }

    struct dirent* current_entry;
    while ((current_entry = readdir(dir)) != nullptr) {
        if (filterMethod(current_entry) == 0) {
            outList.push_back(*current_entry);
        }
    }

    closedir(dir);

    if (sortMethod != nullptr) {
        sort(outList.begin(), outList.end(), sortMethod);
    }

    TT_LOG_I(TAG, "scandir finish");
    return (int)outList.size();
};

bool isSupportedExecutableFile(const std::string& filename) {
#ifdef ESP_PLATFORM
    // Currently only the PNG library is built into Tactility
    return filename.ends_with(".elf");
#else
    return false;
#endif
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

} // namespace tt::app::files
