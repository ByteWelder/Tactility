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
    if (*output == nullptr) {
        TT_LOG_E(TAG, "Out of memory");
        closedir(dir);
        return -1;
    }

    struct dirent** dirent_array = *output;
    int next_dirent_index = 0;

    struct dirent* current_entry;
    bool out_of_memory = false;
    while ((current_entry = readdir(dir)) != nullptr) {
        if (filter(current_entry) == 0) {
            dirent_array[next_dirent_index] = static_cast<dirent*>(malloc(sizeof(struct dirent)));
            if (dirent_array[next_dirent_index] != nullptr) {
                memcpy(dirent_array[next_dirent_index], current_entry, sizeof(struct dirent));

                next_dirent_index++;
                if (next_dirent_index >= SCANDIR_LIMIT) {
                    TT_LOG_E(TAG, "Directory has more than %d files", SCANDIR_LIMIT);
                    break;
                }
            } else {
                TT_LOG_E(TAG, "Alloc failed. Aborting and cleaning up.");
                out_of_memory = true;
                break;
            }
        }
    }

    // Out-of-memory clean-up
    if (out_of_memory && next_dirent_index > 0) {
        for (int i = 0; i < next_dirent_index; ++i) {
            TT_LOG_I(TAG, "Cleanup item %d", i);
            free(dirent_array[i]);
        }
        TT_LOG_I(TAG, "Free");
        free(*output);
        closedir(dir);
        return -1;
    // Empty directory
    } else if (next_dirent_index == 0) {
        free(*output);
        *output = nullptr;
    } else {
        if (sort) {
            qsort(dirent_array, next_dirent_index, sizeof(struct dirent*), (__compar_fn_t)sort);
        }
    }

    closedir(dir);
    return next_dirent_index;
};

}
