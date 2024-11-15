#pragma once

#include <dirent.h>
#include "lvgl.h"

#define MAX_PATH_LENGTH 256

typedef struct {
    char current_path[MAX_PATH_LENGTH];
    struct dirent** dir_entries;
    int dir_entries_count;
    lv_obj_t* list;
} FilesData;

FilesData* files_data_alloc();
void files_data_free(FilesData* data);
void files_data_free_entries(FilesData* data);
bool files_data_set_entries_for_child_path(FilesData* data, const char* child_path);
bool files_data_set_entries_for_path(FilesData* data, const char* path);