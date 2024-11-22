#pragma once

#include <dirent.h>
#include "lvgl.h"

namespace tt::app::files {

#define MAX_PATH_LENGTH 256

typedef struct {
    char current_path[MAX_PATH_LENGTH];
    struct dirent** dir_entries;
    int dir_entries_count;
    lv_obj_t* list;
} Data;

Data* data_alloc();
void data_free(Data* data);
void data_free_entries(Data* data);
bool data_set_entries_for_child_path(Data* data, const char* child_path);
bool data_set_entries_for_path(Data* data, const char* path);

} // namespace
