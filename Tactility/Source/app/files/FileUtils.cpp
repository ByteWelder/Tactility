#include "FileUtils.h"
#include "TactilityCore.h"
#include <cstdlib>
#include <cstring>

namespace tt::app::files {

#define TAG "file_utils"

#define SCANDIR_LIMIT 128

int dirent_filter_dot_entries(const struct dirent* entry) {
    return (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) ? -1 : 0;
}

int dirent_sort_alpha_and_type(const struct dirent** left, const struct dirent** right) {
    bool left_is_dir = (*left)->d_type == TT_DT_DIR || (*left)->d_type == TT_DT_CHR;
    bool right_is_dir = (*right)->d_type == TT_DT_DIR || (*right)->d_type == TT_DT_CHR;
    if (left_is_dir == right_is_dir) {
        return strcmp((*left)->d_name, (*right)->d_name);
    } else {
        return (left_is_dir < right_is_dir) ? 1 : -1;
    }
}

int dirent_sort_alpha(const struct dirent** left, const struct dirent** right) {
    return strcmp((*left)->d_name, (*right)->d_name);
}

int scandir(
    const char* path,
    struct dirent*** output,
    ScandirFilter _Nullable filter,
    ScandirSort _Nullable sort
) {
    DIR* dir = opendir(path);
    if (dir == nullptr) {
        TT_LOG_E(TAG, "Failed to open dir %s", path);
        return -1;
    }

    *output = static_cast<dirent**>(malloc(sizeof(void*) * SCANDIR_LIMIT));
    struct dirent** dirent_array = *output;
    int dirent_buffer_index = 0;

    struct dirent* current_entry;
    while ((current_entry = readdir(dir)) != nullptr) {
        if (filter(current_entry) == 0) {
            dirent_array[dirent_buffer_index] = static_cast<dirent*>(malloc(sizeof(struct dirent)));
            memcpy(dirent_array[dirent_buffer_index], current_entry, sizeof(struct dirent));

            dirent_buffer_index++;
            if (dirent_buffer_index >= SCANDIR_LIMIT) {
                TT_LOG_E(TAG, "Directory has more than %d files", SCANDIR_LIMIT);
                break;
            }
        }
    }

    if (dirent_buffer_index == 0) {
        free(*output);
        *output = nullptr;
    } else {
        if (sort) {
            qsort(dirent_array, dirent_buffer_index, sizeof(struct dirent*), (__compar_fn_t)sort);
        }
    }

    closedir(dir);
    return dirent_buffer_index;
};

}
