#include "files_data.h"

#include "app.h"
#include "apps/system/image_viewer/image_viewer.h"
#include "assets.h"
#include "check.h"
#include "file_utils.h"
#include "lvgl.h"
#include "services/loader/loader.h"
#include "string_utils.h"
#include "ui/toolbar.h"
#include <dirent.h>
#include <unistd.h>

#define TAG "files_app"

/**
 * Case-insensitive check to see if the given file matches the provided file extension.
 * @param path the full path to the file
 * @param extension the extension to look for, including the period symbol, in lower case
 * @return true on match
 */
static bool has_file_extension(const char* path, const char* extension) {
    size_t postfix_len = strlen(extension);
    size_t base_len = strlen(path);
    if (base_len < postfix_len) {
        return false;
    }

    for (int i = (int)postfix_len - 1; i >= 0; i--) {
        if (tolower(path[base_len - postfix_len + i]) != tolower(extension[i])) {
            return false;
        }
    }

    return true;
}

static bool is_supported_image_file(const char* filename) {
    // Currently only the PNG library is built into Tactility
    return has_file_extension(filename, ".png");
}

// region Views

static void update_views(FilesData* data);

static void on_navigate_up_pressed(lv_event_t* event) {
    FilesData* files_data = (FilesData*)lv_event_get_user_data(event);
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

static void view_file(const char* path, const char* filename) {
    size_t path_len = strlen(path);
    size_t filename_len = strlen(filename);
    char* filepath = malloc(path_len + filename_len + 2);
    sprintf(filepath, "%s/%s", path, filename);

    // For PC we need to make the path relative to the current work directory,
    // because that's how LVGL maps its 'drive letter' to the file system.
    char* processed_filepath;
    if (tt_get_platform() == PlatformPc) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            TT_LOG_E(TAG, "Failed to get current working directory");
            return;
        }
        if (!strstr(filepath, cwd)) {
            TT_LOG_E(TAG, "Can only work with files in working directory %s", cwd);
            return;
        }
        char* substr = filepath + strlen(cwd);
        processed_filepath = substr;
    } else {
        processed_filepath = filepath;
    }

    TT_LOG_I(TAG, "Clicked %s", filepath);

    if (is_supported_image_file(filename)) {
        Bundle bundle = tt_bundle_alloc();
        tt_bundle_put_string(bundle, IMAGE_VIEWER_FILE_ARGUMENT, processed_filepath);
        loader_start_app("image_viewer", false, bundle);
    } else {
        TT_LOG_W(TAG, "opening files of this type is not supported");
    }

    free(filepath);
}

static void on_file_pressed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* button = lv_event_get_current_target_obj(event);
        FilesData* files_data = lv_obj_get_user_data(button);

        struct dirent* dir_entry = lv_event_get_user_data(event);
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
                view_file(files_data->current_path, dir_entry->d_name);
                break;
            default:
                // Assume it's a file
                // TODO: Find a better way to identify a file
                view_file(files_data->current_path, dir_entry->d_name);
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
    } else if (is_supported_image_file(dir_entry->d_name)) {
        symbol = LV_SYMBOL_IMAGE;
    } else if (dir_entry->d_type == TT_DT_LNK) {
        symbol = LV_SYMBOL_LOOP;
    } else {
        symbol = LV_SYMBOL_FILE;
    }
    lv_obj_t* button = lv_list_add_button(list, symbol, dir_entry->d_name);
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
    // PC platform is bound to current work directory because of the LVGL file system mapping
    if (tt_get_platform() == PlatformPc) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            files_data_set_entries_for_path(data, cwd);
        } else {
            TT_LOG_E(TAG, "Failed to get current work directory files");
            files_data_set_entries_for_path(data, "/");
        }
    } else {
        files_data_set_entries_for_path(data, "/");
    }

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
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = AppTypeSystem,
    .on_start = &on_start,
    .on_stop = &on_stop,
    .on_show = &on_show,
    .on_hide = NULL
};
