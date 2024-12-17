#include "FilesData.h"
#include "app/AppContext.h"
#include "Tactility.h"
#include "Assets.h"
#include "Check.h"
#include "FileUtils.h"
#include "StringUtils.h"
#include "app/ElfApp.h"
#include "app/imageviewer/ImageViewer.h"
#include "app/textviewer/TextViewer.h"
#include "lvgl.h"
#include "service/loader/Loader.h"
#include "lvgl/Toolbar.h"

#include <dirent.h>
#include <unistd.h>
#include <memory>
#include <cstring>

namespace tt::app::files {

#define TAG "files_app"

extern const AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<Data> _Nullable optData() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return std::static_pointer_cast<Data>(app->getData());
    } else {
        return nullptr;
    }
}

/**
 * Case-insensitive check to see if the given file matches the provided file extension.
 * @param path the full path to the file
 * @param extension the extension to look for, including the period symbol, in lower case
 * @return true on match
 */
static bool hasFileExtension(const char* path, const char* extension) {
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

static bool isSupportedExecutableFile(const char* filename) {
#ifdef ESP_PLATFORM
    // Currently only the PNG library is built into Tactility
    return hasFileExtension(filename, ".elf");
#else
    return false;
#endif
}

static bool isSupportedImageFile(const char* filename) {
    // Currently only the PNG library is built into Tactility
    return hasFileExtension(filename, ".png");
}

static bool isSupportedTextFile(const char* filename) {
    return hasFileExtension(filename, ".txt") ||
           hasFileExtension(filename, ".ini") ||
           hasFileExtension(filename, ".json") ||
           hasFileExtension(filename, ".yaml") ||
           hasFileExtension(filename, ".yml") ||
           hasFileExtension(filename, ".lua") ||
           hasFileExtension(filename, ".js") ||
           hasFileExtension(filename, ".properties");
}

// region Views

static void updateViews(std::shared_ptr<Data> data);

static void onNavigateUpPressed(TT_UNUSED lv_event_t* event) {
    auto files_data = optData();
    if (files_data == nullptr) {
        return;
    }

    if (strcmp(files_data->current_path, "/") != 0) {
        TT_LOG_I(TAG, "Navigating upwards");
        char new_absolute_path[MAX_PATH_LENGTH];
        if (string::getPathParent(files_data->current_path, new_absolute_path)) {
            data_set_entries_for_path(files_data, new_absolute_path);
        }
    }

    updateViews(files_data);
}

static void viewFile(const char* path, const char* filename) {
    size_t path_len = strlen(path);
    size_t filename_len = strlen(filename);
    char* filepath = static_cast<char*>(malloc(path_len + filename_len + 2));
    sprintf(filepath, "%s/%s", path, filename);

    // For PC we need to make the path relative to the current work directory,
    // because that's how LVGL maps its 'drive letter' to the file system.
    char* processed_filepath;
    if (kernel::getPlatform() == kernel::PlatformSimulator) {
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

    if (isSupportedExecutableFile(filename)) {
#ifdef ESP_PLATFORM
        app::startElfApp(processed_filepath);
#endif
    } else if (isSupportedImageFile(filename)) {
        auto bundle = std::make_shared<Bundle>();
        bundle->putString(IMAGE_VIEWER_FILE_ARGUMENT, processed_filepath);
        service::loader::startApp("ImageViewer", false, bundle);
    } else if (isSupportedTextFile(filename)) {
        auto bundle = std::make_shared<Bundle>();
        if (kernel::getPlatform() == kernel::PlatformEsp) {
            bundle->putString(TEXT_VIEWER_FILE_ARGUMENT, processed_filepath);
        } else {
            // Remove forward slash, because we need a relative path
            bundle->putString(TEXT_VIEWER_FILE_ARGUMENT, processed_filepath + 1);
        }
        service::loader::startApp("TextViewer", false, bundle);
    } else {
        TT_LOG_W(TAG, "opening files of this type is not supported");
    }

    free(filepath);
}

static void onFilePressed(lv_event_t* event) {
    auto files_data = optData();
    if (files_data == nullptr) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED) {
        auto* dir_entry = static_cast<dirent*>(lv_event_get_user_data(event));
        TT_LOG_I(TAG, "Pressed %s %d", dir_entry->d_name, dir_entry->d_type);

        switch (dir_entry->d_type) {
            case TT_DT_DIR:
            case TT_DT_CHR:
                data_set_entries_for_child_path(files_data, dir_entry->d_name);
                updateViews(files_data);
                break;
            case TT_DT_LNK:
                TT_LOG_W(TAG, "opening links is not supported");
                break;
            case TT_DT_REG:
                viewFile(files_data->current_path, dir_entry->d_name);
                break;
            default:
                // Assume it's a file
                // TODO: Find a better way to identify a file
                viewFile(files_data->current_path, dir_entry->d_name);
                break;
        }
    }
}

static void createFileWidget(lv_obj_t* parent, struct dirent* dir_entry) {
    tt_check(parent);
    auto* list = (lv_obj_t*)parent;
    const char* symbol;
    if (dir_entry->d_type == TT_DT_DIR || dir_entry->d_type == TT_DT_CHR) {
        symbol = LV_SYMBOL_DIRECTORY;
    } else if (isSupportedImageFile(dir_entry->d_name)) {
        symbol = LV_SYMBOL_IMAGE;
    } else if (dir_entry->d_type == TT_DT_LNK) {
        symbol = LV_SYMBOL_LOOP;
    } else {
        symbol = LV_SYMBOL_FILE;
    }
    lv_obj_t* button = lv_list_add_button(list, symbol, dir_entry->d_name);
    lv_obj_add_event_cb(button, &onFilePressed, LV_EVENT_CLICKED, (void*)dir_entry);
}

static void updateViews(std::shared_ptr<Data> data) {
    lv_obj_clean(data->list);
    for (int i = 0; i < data->dir_entries_count; ++i) {
        TT_LOG_D(TAG, "Entry: %s %d", data->dir_entries[i]->d_name, data->dir_entries[i]->d_type);
        createFileWidget(data->list, data->dir_entries[i]);
    }
}

// endregion Views

// region Lifecycle

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto data = std::static_pointer_cast<Data>(app.getData());

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* toolbar = lvgl::toolbar_create(parent, "Files");
    lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_UP, &onNavigateUpPressed, nullptr);

    data->list = lv_list_create(parent);
    lv_obj_set_width(data->list, LV_PCT(100));
    lv_obj_set_flex_grow(data->list, 1);

    updateViews(data);
}

static void onStart(AppContext& app) {
    auto* test = new uint32_t;
    delete test;
    auto data = std::make_shared<Data>();
    // PC platform is bound to current work directory because of the LVGL file system mapping
    if (kernel::getPlatform() == kernel::PlatformSimulator) {
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

// endregion Lifecycle

extern const AppManifest manifest = {
    .id = "Files",
    .name = "Files",
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = TypeHidden,
    .onStart = onStart,
    .onShow = onShow,
};

} // namespace
