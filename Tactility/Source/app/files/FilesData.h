#pragma once

#include "lvgl.h"
#include <dirent.h>
#include <memory>

namespace tt::app::files {

#define MAX_PATH_LENGTH 256

struct Data {
    char current_path[MAX_PATH_LENGTH] = { 0 };
    struct dirent** dir_entries = nullptr;
    int dir_entries_count = 0;
    lv_obj_t* list = nullptr;

    void freeEntries() {
        for (int i = 0; i < dir_entries_count; ++i) {
            free(dir_entries[i]);
        }
        free(dir_entries);
        dir_entries = nullptr;
        dir_entries_count = 0;
    }

    ~Data() {
        freeEntries();
    }
};

void data_free(std::shared_ptr<Data> data);
void data_free_entries(std::shared_ptr<Data> data);
bool data_set_entries_for_child_path(std::shared_ptr<Data> data, const char* child_path);
bool data_set_entries_for_path(std::shared_ptr<Data> data, const char* path);

} // namespace
