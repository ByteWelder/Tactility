#include "files_data.h"
#include "file_utils.h"
#include "tactility_core.h"
#include <string_utils.h>

#define TAG "files_app"

static bool get_child_path(char* base_path, const char* child_path, char* out_path, size_t out_size) {
    size_t current_path_length = strlen(base_path);
    size_t added_path_length = strlen(child_path);
    size_t total_path_length = current_path_length + added_path_length + 1; // two paths with `/`

    if (total_path_length >= out_size) {
        TT_LOG_E(TAG, "Path limit reached (%d chars)", MAX_PATH_LENGTH);
        return false;
    } else {
        memcpy(out_path, base_path, current_path_length);
        // Postfix with "/" when the current path isn't "/"
        if (current_path_length != 1) {
            out_path[current_path_length] = '/';
            strcpy(&out_path[current_path_length + 1], child_path);
        } else {
            strcpy(&out_path[current_path_length], child_path);
        }
        return true;
    }
}

FilesData* files_data_alloc() {
    FilesData* data = malloc(sizeof(FilesData));
    *data = (FilesData) {
        .current_path = { 0x00 },
        .dir_entries = NULL,
        .dir_entries_count = 0
    };
    return data;
}

void files_data_free(FilesData* data) {
    files_data_free_entries(data);
    free(data);
}

void files_data_free_entries(FilesData* data) {
    for (int i = 0; i < data->dir_entries_count; ++i) {
        free(data->dir_entries[i]);
    }
    free(data->dir_entries);
    data->dir_entries = NULL;
    data->dir_entries_count = 0;
}

static void files_data_set_entries(FilesData* data, struct dirent** entries, int count) {
    if (data->dir_entries != NULL) {
        files_data_free_entries(data);
    }

    data->dir_entries = entries;
    data->dir_entries_count = count;
}

bool files_data_set_entries_for_path(FilesData* data, const char* path) {
    TT_LOG_I(TAG, "Changing path: %s -> %s", data->current_path, path);

    /**
     * ESP32 does not have a root directory, so we have to create it manually.
     * We'll add the NVS Flash partitions and the binding for the sdcard.
     */
    if (tt_get_platform() == PLATFORM_ESP && strcmp(path, "/") == 0) {
        int dir_entries_count = 3;
        struct dirent** dir_entries = malloc(sizeof(struct dirent*) * 3);

        dir_entries[0] = malloc(sizeof(struct dirent));
        dir_entries[0]->d_type = TT_DT_DIR;
        strcpy(dir_entries[0]->d_name, "assets");

        dir_entries[1] = malloc(sizeof(struct dirent));
        dir_entries[1]->d_type = TT_DT_DIR;
        strcpy(dir_entries[1]->d_name, "config");

        dir_entries[2] = malloc(sizeof(struct dirent));
        dir_entries[2]->d_type = TT_DT_DIR;
        strcpy(dir_entries[2]->d_name, "sdcard");

        files_data_set_entries(data, dir_entries, dir_entries_count);
        strcpy(data->current_path, path);
        return true;
    } else {
        struct dirent** entries = NULL;
        int count = tt_scandir(path, &entries, &tt_dirent_filter_dot_entries, &tt_dirent_sort_alpha_and_type);
        if (count >= 0) {
            TT_LOG_I(TAG, "%s has %u entries", path, count);
            files_data_set_entries(data, entries, count);
            strcpy(data->current_path, path);
            return true;
        } else {
            TT_LOG_E(TAG, "Failed to fetch entries for %s", path);
            return false;
        }
    }
}

bool files_data_set_entries_for_child_path(FilesData* data, const char* child_path) {
    char new_absolute_path[MAX_PATH_LENGTH];
    if (get_child_path(data->current_path, child_path, new_absolute_path, MAX_PATH_LENGTH)) {
        TT_LOG_I(TAG, "Navigating from %s to %s", data->current_path, new_absolute_path);
        return files_data_set_entries_for_path(data, new_absolute_path);
    } else {
        TT_LOG_I(TAG, "Failed to get child path for %s/%s", data->current_path, child_path);
        return false;
    }
}
