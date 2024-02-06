#include "file_utils.h"
#include "tactility_core.h"

#define TAG "file_utils"

#define SCANDIR_LIMIT 128

int tt_dirent_filter_dot_entries(const struct dirent* entry) {
    return (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) ? -1 : 0;
}

int tt_dirent_sort_alpha_and_type(const struct dirent** left, const struct dirent** right) {
    bool left_is_dir = (*left)->d_type == TT_DT_DIR;
    bool right_is_dir = (*right)->d_type == TT_DT_DIR;
    if (left_is_dir == right_is_dir) {
        return strcmp((*left)->d_name, (*right)->d_name);
    } else {
        return (left_is_dir < right_is_dir) ? 1 : -1;
    }
}

int tt_dirent_sort_alpha(const struct dirent** left, const struct dirent** right) {
    return strcmp((*left)->d_name, (*right)->d_name);
}

int tt_scandir(
    const char* path,
    struct dirent*** output,
    ScandirFilter _Nullable filter,
    ScandirSort _Nullable sort
) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    *output = malloc(sizeof(void*) * SCANDIR_LIMIT);
    struct dirent** dirent_array = *output;
    int dirent_buffer_index = 0;

    struct dirent* current_entry;
    while ((current_entry = readdir(dir)) != NULL) {
        TT_LOG_D(TAG, "debug: %s %d", current_entry->d_name, current_entry->d_type);
        if (filter(current_entry) == 0) {
            dirent_array[dirent_buffer_index] = malloc(sizeof(struct dirent));
            memcpy(dirent_array[dirent_buffer_index], current_entry, sizeof(struct dirent));

            dirent_buffer_index++;
            if (dirent_buffer_index >= SCANDIR_LIMIT) {
                TT_LOG_E(TAG, "directory has more than %d files", SCANDIR_LIMIT);
                break;
            }
        }
    }

    if (dirent_buffer_index == 0) {
        free(*output);
        *output = NULL;
    } else {
        if (sort) {
            qsort(dirent_array, dirent_buffer_index, sizeof(struct dirent*), (__compar_fn_t)sort);
        }
    }

    closedir(dir);
    return dirent_buffer_index;
};
