#include "Tactility/app/fileselection/View.h"

#include "Tactility/app/alertdialog/AlertDialog.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/lvgl/LvglSync.h"

#include <Tactility/Tactility.h>
#include "Tactility/file/File.h"
#include <Tactility/StringUtils.h>

#include <cstring>
#include <unistd.h>

#ifdef ESP_PLATFORM
#include "Tactility/service/loader/Loader.h"
#endif

#define TAG "fileselection_app"

namespace tt::app::fileselection {

// region Callbacks

static void dirEntryListScrollBeginCallback(lv_event_t* event) {
    auto* view = static_cast<View*>(lv_event_get_user_data(event));
    view->onDirEntryListScrollBegin();
}

static void onDirEntryPressedCallback(lv_event_t* event) {
    auto* view = static_cast<View*>(lv_event_get_user_data(event));
    auto* button = lv_event_get_target_obj(event);
    auto index = lv_obj_get_index(button);
    view->onDirEntryPressed(index);
}

static void onNavigateUpPressedCallback(TT_UNUSED lv_event_t* event) {
    auto* view = static_cast<View*>(lv_event_get_user_data(event));
    view->onNavigateUpPressed();
}

// endregion

void View::viewFile(const std::string& path, const std::string& filename) {
    std::string file_path = path + "/" + filename;

    // For PC we need to make the path relative to the current work directory,
    // because that's how LVGL maps its 'drive letter' to the file system.
    std::string processed_filepath;
    if (kernel::getPlatform() == kernel::PlatformSimulator) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            TT_LOG_E(TAG, "Failed to get current working directory");
            return;
        }
        if (!file_path.starts_with(cwd)) {
            TT_LOG_E(TAG, "Can only work with files in working directory %s", cwd);
            return;
        }
        processed_filepath = file_path.substr(strlen(cwd));
    } else {
        processed_filepath = file_path;
    }

    TT_LOG_I(TAG, "Clicked %s", file_path.c_str());

    onFileSelected(file_path);

    onNavigate();
}

void View::onDirEntryPressed(uint32_t index) {
    dirent dir_entry;
    if (state->getDirent(index, dir_entry)) {
        TT_LOG_I(TAG, "Pressed %s %d", dir_entry.d_name, dir_entry.d_type);
        state->setSelectedChildEntry(dir_entry.d_name);
        using namespace tt::file;
        switch (dir_entry.d_type) {
            case TT_DT_DIR:
            case TT_DT_CHR:
                state->setEntriesForChildPath(dir_entry.d_name);
                onNavigate();
                update();
                break;
            case TT_DT_LNK:
                TT_LOG_W(TAG, "opening links is not supported");
                break;
            case TT_DT_REG:
                viewFile(state->getCurrentPath(), dir_entry.d_name);
                onNavigate();
                break;
            default:
                // Assume it's a file
                // TODO: Find a better way to identify a file
                viewFile(state->getCurrentPath(), dir_entry.d_name);
                onNavigate();
                break;
        }
    }
}

void View::createDirEntryWidget(lv_obj_t* list, dirent& dir_entry) {
    tt_check(list);
    const char* symbol;
    if (dir_entry.d_type == file::TT_DT_DIR || dir_entry.d_type == file::TT_DT_CHR) {
        symbol = LV_SYMBOL_DIRECTORY;
    } else {
        symbol = LV_SYMBOL_FILE;
    }
    lv_obj_t* button = lv_list_add_button(list, symbol, dir_entry.d_name);
    lv_obj_add_event_cb(button, &onDirEntryPressedCallback, LV_EVENT_SHORT_CLICKED, this);
}

void View::onNavigateUpPressed() {
    if (state->getCurrentPath() != "/") {
        TT_LOG_I(TAG, "Navigating upwards");
        std::string new_absolute_path;
        if (string::getPathParent(state->getCurrentPath(), new_absolute_path)) {
            state->setEntriesForPath(new_absolute_path);
        }
        onNavigate();
        update();
    }
}

void View::update() {
    auto scoped_lockable = lvgl::getSyncLock()->asScopedLock();
    if (scoped_lockable.lock(lvgl::defaultLockTime)) {
        lv_obj_clean(dir_entry_list);

        state->withEntries([this](const std::vector<dirent>& entries) {
            for (auto entry : entries) {
                TT_LOG_D(TAG, "Entry: %s %d", entry.d_name, entry.d_type);
                createDirEntryWidget(dir_entry_list, entry);
            }
        });

        if (state->getCurrentPath() == "/") {
            lv_obj_add_flag(navigate_up_button, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(navigate_up_button, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "lvgl");
    }
}

void View::init(lv_obj_t* parent, Mode mode) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    auto* toolbar = lvgl::toolbar_create(parent, "Select File");
    navigate_up_button = lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_UP, &onNavigateUpPressedCallback, this);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);

    dir_entry_list = lv_list_create(wrapper);
    lv_obj_set_height(dir_entry_list, LV_PCT(100));
    lv_obj_set_flex_grow(dir_entry_list, 1);

    lv_obj_add_event_cb(dir_entry_list, dirEntryListScrollBeginCallback, LV_EVENT_SCROLL_BEGIN, this);

    action_list = lv_list_create(wrapper);
    lv_obj_set_height(action_list, LV_PCT(100));
    lv_obj_set_flex_grow(action_list, 1);
    lv_obj_add_flag(action_list, LV_OBJ_FLAG_HIDDEN);

    update();
}

void View::onDirEntryListScrollBegin() {
    auto scoped_lockable = lvgl::getSyncLock()->asScopedLock();
    if (scoped_lockable.lock(lvgl::defaultLockTime)) {
        lv_obj_add_flag(action_list, LV_OBJ_FLAG_HIDDEN);
    }
}

void View::onNavigate() {
    auto scoped_lockable = lvgl::getSyncLock()->asScopedLock();
    if (scoped_lockable.lock(lvgl::defaultLockTime)) {
        lv_obj_add_flag(action_list, LV_OBJ_FLAG_HIDDEN);
    }
}

}
