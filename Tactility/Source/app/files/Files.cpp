#include "FilesData.h"
#include "Tactility.h"
#include "app/App.h"
#include "Assets.h"
#include "Check.h"
#include "FileUtils.h"
#include "StringUtils.h"
#include "app/imageviewer/ImageViewer.h"
#include "app/textviewer/TextViewer.h"
#include "lvgl.h"
#include "service/loader/Loader.h"
#include "lvgl/Toolbar.h"
#include <dirent.h>
#include <unistd.h>

namespace tt::app::files {

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

static bool is_supported_text_file(const char* filename) {
    return has_file_extension(filename, ".txt") ||
        has_file_extension(filename, ".ini") ||
        has_file_extension(filename, ".json") ||
        has_file_extension(filename, ".yaml") ||
        has_file_extension(filename, ".yml") ||
        has_file_extension(filename, ".lua") ||
        has_file_extension(filename, ".js") ||
        has_file_extension(filename, ".properties");
}

// region Views

static void update_views(Data* data);

static void on_navigate_up_pressed(lv_event_t* event) {
    auto* files_data = (Data*)lv_event_get_user_data(event);
    if (strcmp(files_data->current_path, "/") != 0) {
        TT_LOG_I(TAG, "Navigating upwards");
        char new_absolute_path[MAX_PATH_LENGTH];
        if (string_get_path_parent(files_data->current_path, new_absolute_path)) {
            data_set_entries_for_path(files_data, new_absolute_path);
        }
    }
    update_views(files_data);
}

static void on_exit_app_pressed(TT_UNUSED lv_event_t* event) {
    service::loader::stopApp();
}

static void view_file(const char* path, const char* filename) {
    size_t path_len = strlen(path);
    size_t filename_len = strlen(filename);
    char* filepath = static_cast<char*>(malloc(path_len + filename_len + 2));
    sprintf(filepath, "%s/%s", path, filename);

    // For PC we need to make the path relative to the current work directory,
    // because that's how LVGL maps its 'drive letter' to the file system.
    char* processed_filepath;
    if (get_platform() == PlatformSimulator) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
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
        Bundle bundle;
        bundle.putString(IMAGE_VIEWER_FILE_ARGUMENT, processed_filepath);
        service::loader::startApp("ImageViewer", false, bundle);
    } else if (is_supported_text_file(filename)) {
        Bundle bundle;
        if (get_platform() == PlatformEsp) {
            bundle.putString(TEXT_VIEWER_FILE_ARGUMENT, processed_filepath);
        } else {
            // Remove forward slash, because we need a relative path
            bundle.putString(TEXT_VIEWER_FILE_ARGUMENT, processed_filepath + 1);
        }
        service::loader::startApp("TextViewer", false, bundle);
    } else {
        TT_LOG_W(TAG, "opening files of this type is not supported");
    }

    free(filepath);
}

static void on_file_pressed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* button = lv_event_get_current_target_obj(event);
        auto* files_data = static_cast<Data*>(lv_obj_get_user_data(button));

        auto* dir_entry = static_cast<dirent*>(lv_event_get_user_data(event));
        TT_LOG_I(TAG, "Pressed %s %d", dir_entry->d_name, dir_entry->d_type);

        switch (dir_entry->d_type) {
            case TT_DT_DIR:
                data_set_entries_for_child_path(files_data, dir_entry->d_name);
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

static void create_file_widget(Data* files_data, lv_obj_t* parent, struct dirent* dir_entry) {
    tt_check(parent);
    auto* list = (lv_obj_t*)parent;
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

static void update_views(Data* data) {
    lv_obj_clean(data->list);
    for (int i = 0; i < data->dir_entries_count; ++i) {
        create_file_widget(data, data->list, data->dir_entries[i]);
    }
}

// endregion Views

// region Lifecycle

static void on_show(App& app, lv_obj_t* parent) {
    auto* data = static_cast<Data*>(app.getData());

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* toolbar = lvgl::toolbar_create(parent, "Files");
    lvgl::toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, &on_exit_app_pressed, nullptr);
    lvgl::toolbar_add_action(toolbar, LV_SYMBOL_UP, "Navigate up", &on_navigate_up_pressed, data);

    data->list = lv_list_create(parent);
    lv_obj_set_width(data->list, LV_PCT(100));
    lv_obj_set_flex_grow(data->list, 1);

    update_views(data);
}

static void on_start(App& app) {
    auto* data = data_alloc();
    // PC platform is bound to current work directory because of the LVGL file system mapping
    if (get_platform() == PlatformSimulator) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            data_set_entries_for_path(data, cwd);
        } else {
            TT_LOG_E(TAG, "Failed to get current work directory files");
            data_set_entries_for_path(data, "/");
        }
    } else {
        data_set_entries_for_path(data, "/");
    }

    app.setData(data);
}

static void on_stop(App& app) {
    auto* data = static_cast<Data*>(app.getData());
    data_free(data);
}

// endregion Lifecycle

extern const Manifest manifest = {
    .id = "Files",
    .name = "Files",
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = TypeSystem,
    .onStart = &on_start,
    .onStop = &on_stop,
    .onShow = &on_show,
};

} // namespace
