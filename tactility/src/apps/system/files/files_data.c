#include "files_data.h"
#include "file_utils.h"
#include "tactility_core.h"
#include <string_utils.h>

#define TAG "files_app"

FilesData* files_data_alloc() {
    FilesData* data = malloc(sizeof(FilesData));
    *data = (FilesData) {
        .current_path = {'/', 0x00},
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

void files_data_set_entries(FilesData* data, struct dirent** entries, int count) {
    if (data->dir_entries != NULL) {
        files_data_free_entries(data);
    }

    data->dir_entries = entries;
    data->dir_entries_count = count;
}


void files_data_set_entries_for_path(FilesData* data, const char* path) {
    size_t current_path_length = strlen(data->current_path);
    size_t added_path_length = strlen(path);
    size_t total_path_length = current_path_length + added_path_length + 1; // two paths with `/`

    if (total_path_length >= MAX_PATH_LENGTH) {
        TT_LOG_E(TAG, "path limit reached (%d chars)", MAX_PATH_LENGTH);
        return;
    }

    if (strcmp(path, "..") == 0) {
        char new_absolute_path[MAX_PATH_LENGTH];
        if (tt_string_get_path_parent(data->current_path, new_absolute_path)) {
            if (data->dir_entries != NULL) {
                files_data_free_entries(data);
            }
            if (new_absolute_path[0] == 0x00) {
                files_data_set_entries_root(data);
            } else {
                strcpy(data->current_path, new_absolute_path);
                data->dir_entries_count = tt_scandir(new_absolute_path, &(data->dir_entries), &tt_dirent_filter_dot_entries, &tt_dirent_sort_alpha_and_type);
                TT_LOG_I(TAG, "%s has %u entries", new_absolute_path, data->dir_entries_count);
            }
        }
    } else if (strcmp(path, ".") == 0) {

    } else {
        if (data->dir_entries != NULL) {
            files_data_free_entries(data);
        }

        char new_absolute_path[MAX_PATH_LENGTH];
        memcpy(new_absolute_path, data->current_path, current_path_length);
        // Postfix with "/" when the current path isn't "/"
        if (current_path_length != 1) {
            new_absolute_path[current_path_length] = '/';
            strcpy(&new_absolute_path[current_path_length + 1], path);
        } else {
            strcpy(&new_absolute_path[current_path_length], path);
        }
        strcpy(data->current_path, new_absolute_path);

        data->dir_entries_count = tt_scandir(new_absolute_path, &(data->dir_entries), &tt_dirent_filter_dot_entries, &tt_dirent_sort_alpha_and_type);
        TT_LOG_I(TAG, "%s has %u entries", new_absolute_path, data->dir_entries_count);
    }
}

void files_data_set_entries_root(FilesData* data) {
    tt_assert(data->dir_entries == NULL && data->dir_entries_count == 0U);

    data->current_path[0] = '/';
    data->current_path[1] = 0x00;

#ifdef ESP_PLATFORM
    int dir_entries_count = 3;
    struct dirent** dir_entries = malloc(sizeof(struct dirent*) * 3);

    dir_entries[0] = malloc(sizeof(struct dirent));
    dir_entries[0]->d_type = 4;
    strcpy(dir_entries[0]->d_name, "assets");

    dir_entries[1] = malloc(sizeof(struct dirent));
    dir_entries[1]->d_type = 4;
    strcpy(dir_entries[1]->d_name, "config");

    dir_entries[2] = malloc(sizeof(struct dirent));
    dir_entries[2]->d_type = 4;
    strcpy(dir_entries[2]->d_name, "sdcard");

    files_data_set_entries(data, dir_entries, dir_entries_count);
#else
    data->dir_entries_count = tt_scandir(data->current_path, &(data->dir_entries), &tt_dirent_filter_dot_entries, &tt_dirent_sort_alpha_and_type);
#endif
}
