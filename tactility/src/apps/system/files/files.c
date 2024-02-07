#include "files_data.h"

#include "app.h"
#include "check.h"
#include "file_utils.h"
#include "lvgl.h"
#include "services/loader/loader.h"
#include "ui/toolbar.h"
#include <dirent.h>
#include <string_utils.h>

#define TAG "files_app"

bool tt_string_ends_with(const char* base, const char* postfix) {
    size_t postfix_len = strlen(postfix);
    size_t base_len = strlen(base);
    if (base_len < postfix_len) {
        return false;
    }

    for (int i = (int)postfix_len - 1; i >= 0; i--) {
        if (tolower(base[base_len - postfix_len + i]) != postfix[i]) {
            return false;
        }
    }

    return true;
}

static bool is_image_file(const char* filename) {
    return tt_string_ends_with(filename, ".jpg") ||
        tt_string_ends_with(filename, ".png") ||
        tt_string_ends_with(filename, ".jpeg") ||
        tt_string_ends_with(filename, ".svg") ||
        tt_string_ends_with(filename, ".bmp");
}

// region Views

static void update_views(FilesData* data);

static void on_navigate_up_pressed(TT_UNUSED lv_event_t* event) {
    FilesData* files_data = (FilesData*)event->user_data;
    if (strcmp(files_data->current_path, "/") != 0) {
        TT_LOG_I(TAG, "Navigating upwards");
        char new_absolute_path[MAX_PATH_LENGTH];
        if (tt_string_get_path_parent(files_data->current_path, new_absolute_path)) {
            files_data_set_entries_for_path(files_data, new_absolute_path);
        }
    }
    update_views(files_data);
}

static void on_exit_app_pressed(TT_UNUSED lv_event_t* event) {
    loader_stop_app();
}

static void on_file_pressed(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* button = e->current_target;
        FilesData* files_data = lv_obj_get_user_data(button);

        struct dirent* dir_entry = e->user_data;
        TT_LOG_I(TAG, "Pressed %s %d", dir_entry->d_name, dir_entry->d_type);

        switch (dir_entry->d_type) {
            case TT_DT_DIR:
                files_data_set_entries_for_child_path(files_data, dir_entry->d_name);
                update_views(files_data);
                break;
            case TT_DT_LNK:
                TT_LOG_W(TAG, "opening links is not supported");
                break;
            case TT_DT_REG:
                TT_LOG_W(TAG, "opening files is not supported");
                break;
            default:
                TT_LOG_W(TAG, "file type %d is not supported", dir_entry->d_type);
                break;
        }
    }
}

static void create_file_widget(FilesData* files_data, lv_obj_t* parent, struct dirent* dir_entry) {
    tt_check(parent);
    lv_obj_t* list = (lv_obj_t*)parent;
    const char* symbol;
    if (dir_entry->d_type == TT_DT_DIR) {
        symbol = LV_SYMBOL_DIRECTORY;
    } else if (is_image_file(dir_entry->d_name)) {
        symbol = LV_SYMBOL_IMAGE;
    } else if (dir_entry->d_type == TT_DT_LNK) {
        symbol = LV_SYMBOL_LOOP;
    } else {
        symbol = LV_SYMBOL_SETTINGS;
    }
    lv_obj_t* button = lv_list_add_btn(list, symbol, dir_entry->d_name);
    lv_obj_set_user_data(button, files_data);
    lv_obj_add_event_cb(button, &on_file_pressed, LV_EVENT_CLICKED, (void*)dir_entry);
}

static void update_views(FilesData* data) {
    lv_obj_clean(data->list);
    for (int i = 0; i < data->dir_entries_count; ++i) {
        create_file_widget(data, data->list, data->dir_entries[i]);
    }
}

// endregion Views

// region Lifecycle

static void on_show(App app, lv_obj_t* parent) {
    FilesData* data = tt_app_get_data(app);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* toolbar = tt_toolbar_create(parent, "Files");
    tt_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, &on_exit_app_pressed, NULL);
    tt_toolbar_add_action(toolbar, LV_SYMBOL_UP, "Navigate up", &on_navigate_up_pressed, data);

    data->list = lv_list_create(parent);
    lv_obj_set_width(data->list, LV_PCT(100));
    lv_obj_set_flex_grow(data->list, 1);

    update_views(data);
}

static void on_start(App app) {
    FilesData* data = files_data_alloc();
    files_data_set_entries_for_path(data, "/");
    tt_app_set_data(app, data);
}

static void on_stop(App app) {
    FilesData* data = tt_app_get_data(app);
    files_data_free(data);
}

// endregion Lifecycle

const AppManifest files_app = {
    .id = "files",
    .name = "Files",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = &on_start,
    .on_stop = &on_stop,
    .on_show = &on_show,
    .on_hide = NULL
};
