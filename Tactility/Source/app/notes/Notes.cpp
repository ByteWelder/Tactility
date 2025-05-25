#include <Tactility/app/AppManifest.h>
#include <Tactility/file/File.h>
#include <Tactility/lvgl/Keyboard.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Assets.h>
#include <lvgl.h>

#include <Tactility/app/fileselection/FileSelection.h>
#include <Tactility/lvgl/LvglSync.h>

namespace tt::app::notes {

constexpr const char* TAG = "Notes";

class NotesApp : public App {

    AppContext* appContext = nullptr;

    lv_obj_t* uiCurrentFileName;
    lv_obj_t* uiDropDownMenu;
    lv_obj_t* uiNoteText;
    lv_obj_t* uiMessageBox;
    lv_obj_t* uiMessageBoxButtonOk;
    lv_obj_t* uiMessageBoxButtonNo;

    std::string filePath;
    std::string saveBuffer;

    LaunchId loadFileLaunchId = 0;
    LaunchId saveFileLaunchId = 0;

#pragma region Main_Events_Functions

    void appNotesEventCb(lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* obj = lv_event_get_target_obj(e);

        if (code == LV_EVENT_CLICKED) {
            if (obj == uiMessageBoxButtonOk || obj == uiMessageBoxButtonNo) {
                lv_obj_del(uiMessageBox);
            }
        }

        if (code == LV_EVENT_VALUE_CHANGED) {
            if (obj == uiDropDownMenu) {
                switch (lv_dropdown_get_selected(obj)) {
                    case 0: // New
                        resetFileContent();
                        break;
                    case 1: // Save
                        if (!filePath.empty()) {
                            lvgl::getSyncLock()->lock();
                            saveBuffer = lv_textarea_get_text(uiNoteText);
                            lvgl::getSyncLock()->unlock();
                            saveFile(filePath);
                        }
                        break;
                    case 2: // Save as...
                        lvgl::getSyncLock()->lock();
                        saveBuffer = lv_textarea_get_text(uiNoteText);
                        lvgl::getSyncLock()->unlock();
                        saveFileLaunchId = fileselection::startForExistingOrNewFile();
                        TT_LOG_I(TAG, "launched with id %d", loadFileLaunchId);
                        break;
                    case 3: // Load
                        loadFileLaunchId = fileselection::startForExistingFile();
                        TT_LOG_I(TAG, "launched with id %d", loadFileLaunchId);
                        break;
                }
            } else {
                auto* cont = lv_event_get_current_target_obj(e);
                if (obj == cont) return;
                if (lv_obj_get_child(cont, 1)) {
                    saveFileLaunchId = fileselection::startForExistingOrNewFile();
                    TT_LOG_I(TAG, "launched with id %d", loadFileLaunchId);
                } else { //Reset
                    resetFileContent();
                }
                lv_obj_delete(uiMessageBox);
            }
        }
    }

    void resetFileContent() {
        lv_textarea_set_text(uiNoteText, "");
        filePath = "";
        saveBuffer = "";
        lv_label_set_text(uiCurrentFileName, "Untitled");
    }

    void uiMessageBoxShow(std::string title, std::string message, bool isSelectable) {
        uiMessageBox = lv_obj_create(lv_scr_act());
        lv_obj_set_size(uiMessageBox, lv_display_get_horizontal_resolution(nullptr), lv_display_get_vertical_resolution(nullptr));
        lv_obj_align(uiMessageBox, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(uiMessageBox, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* uiMessageBoxTitle = lv_label_create(uiMessageBox);
        lv_label_set_text(uiMessageBoxTitle, title.c_str());
        lv_obj_set_size(uiMessageBoxTitle, lv_display_get_horizontal_resolution(nullptr) - 30, 30);
        lv_obj_align(uiMessageBoxTitle, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* messageLabel = lv_label_create(uiMessageBox);
        lv_obj_align(messageLabel, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_width(messageLabel, LV_PCT(80));
        lv_obj_set_style_text_align(messageLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(messageLabel, message.c_str());
        lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);

        lv_obj_t* buttonWrapper = lv_obj_create(uiMessageBox);
        lv_obj_set_flex_flow(buttonWrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(buttonWrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(buttonWrapper, 0, 0);
        lv_obj_set_flex_align(buttonWrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(buttonWrapper, 0, 0);
        lv_obj_align(buttonWrapper, LV_ALIGN_BOTTOM_MID, 0, 5);

        if (isSelectable == true) {
            lv_obj_t* buttonYes = lv_button_create(buttonWrapper);
            lv_obj_t* buttonLabelYes = lv_label_create(buttonYes);
            lv_obj_align(buttonLabelYes, LV_ALIGN_BOTTOM_LEFT, 0, 0);
            lv_label_set_text(buttonLabelYes, "Yes");
            lv_obj_add_event_cb(buttonYes, [](lv_event_t* e) {
                    auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                    self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);

            uiMessageBoxButtonNo = lv_button_create(buttonWrapper);
            lv_obj_t* buttonLabelNo = lv_label_create(uiMessageBoxButtonNo);
            lv_obj_align(buttonLabelNo, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
            lv_label_set_text(buttonLabelNo, "No");
            lv_obj_add_event_cb(uiMessageBoxButtonNo, [](lv_event_t* e) {
                    auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                    self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);
        } else {
            uiMessageBoxButtonOk = lv_button_create(buttonWrapper);
            lv_obj_t* buttonLabelOk = lv_label_create(uiMessageBoxButtonOk);
            lv_obj_align(buttonLabelOk, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_label_set_text(buttonLabelOk, "Ok");
            lv_obj_add_event_cb(uiMessageBoxButtonOk,
                [](lv_event_t* e) {
                    auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                    self->appNotesEventCb(e);
                },
                LV_EVENT_CLICKED,
                this
            );
        }

        if (!filePath.empty()) {
            openFile(filePath);
        }
    }

#pragma region Open_Events_Functions

    void openFile(const std::string& path) {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        lock.lock();

        // TODO: also use SD card lock, if needed

        auto data = file::readString(path);
        if (data != nullptr) {
            lv_textarea_set_text(uiNoteText, reinterpret_cast<const char*>(data.get()));
            lv_label_set_text(uiCurrentFileName, path.c_str());
            filePath = path;
            TT_LOG_I(TAG, "Loaded from %s", path.c_str());
        }
    }

    bool saveFile(const std::string& path) {
        // TODO: also use SD card lock, if needed
        auto lock = lvgl::getSyncLock()->asScopedLock();
        lock.lock();
        if (file::writeString(path, saveBuffer.c_str())) {
            TT_LOG_I(TAG, "Saved to %s", path.c_str());
            filePath = path;
            return true;
        } else {
            return false;
        }
    }

#pragma endregion Open_Events_Functions

    void onShow(AppContext& context, lv_obj_t* parent) override {
        appContext = &context;

        lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        uiDropDownMenu = lv_dropdown_create(toolbar);
        lv_dropdown_set_options(uiDropDownMenu, LV_SYMBOL_FILE " New File\n" LV_SYMBOL_SAVE " Save\n" LV_SYMBOL_SAVE " Save As...\n" LV_SYMBOL_DIRECTORY " Open File");
        lv_dropdown_set_text(uiDropDownMenu, "Menu");
        lv_dropdown_set_symbol(uiDropDownMenu, LV_SYMBOL_DOWN);
        lv_dropdown_set_selected_highlight(uiDropDownMenu, false);
        lv_obj_set_style_border_color(uiDropDownMenu, lv_color_hex(0xFAFAFA), LV_PART_MAIN);
        lv_obj_set_style_border_width(uiDropDownMenu, 1, LV_PART_MAIN);
        lv_obj_align(uiDropDownMenu, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(uiDropDownMenu,
            [](lv_event_t* e) {
                auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                self->appNotesEventCb(e);
            },
            LV_EVENT_VALUE_CHANGED,
            this
        );

        lv_obj_t* wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_grow(wrapper, 1);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_height(wrapper, LV_PCT(100));
        lv_obj_set_style_pad_all(wrapper, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(wrapper, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

        uiNoteText = lv_textarea_create(wrapper);
        lv_obj_set_width(uiNoteText, LV_PCT(100));
        lv_obj_set_height(uiNoteText, LV_PCT(86));
        lv_textarea_set_password_mode(uiNoteText, false);
        lv_obj_set_style_bg_color(uiNoteText, lv_color_hex(0x262626), LV_PART_MAIN);
        lv_textarea_set_placeholder_text(uiNoteText, "Notes...");

        lv_obj_t* footer = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(footer, lv_color_hex(0x262626), LV_PART_MAIN);
        lv_obj_set_width(footer, LV_PCT(100));
        lv_obj_set_height(footer, LV_PCT(14));
        lv_obj_set_style_pad_all(footer, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(footer, 0, 0);
        lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

        uiCurrentFileName = lv_label_create(footer);
        lv_label_set_long_mode(uiCurrentFileName, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_set_width(uiCurrentFileName, LV_SIZE_CONTENT);
        lv_obj_set_height(uiCurrentFileName, LV_SIZE_CONTENT);
        lv_label_set_text(uiCurrentFileName, "Untitled");
        lv_obj_align(uiCurrentFileName, LV_ALIGN_CENTER, 0, 0);

        //TODO: Move this to SD Card?
        if (!file::findOrCreateDirectory(context.getPaths()->getDataDirectory(), 0777)) {
            TT_LOG_E(TAG, "Failed to find or create path %s", context.getPaths()->getDataDirectory().c_str());
        }

        lvgl::keyboard_add_textarea(uiNoteText);
    }

    void onResult(AppContext& appContext, LaunchId launchId, Result result, std::unique_ptr<Bundle> resultData) override {
        TT_LOG_I(TAG, "Result for launch id %d", launchId);
        if (launchId == loadFileLaunchId) {
            loadFileLaunchId = 0;
            if (result == Result::Ok && resultData != nullptr) {
                auto path = fileselection::getResultPath(*resultData);
                openFile(path);
            }
        } else if (launchId == saveFileLaunchId) {
            saveFileLaunchId = 0;
            if (result == Result::Ok && resultData != nullptr) {
                auto path = fileselection::getResultPath(*resultData);
                // Must re-open file, because UI was cleared after opening other app
                if (saveFile(path)) {
                    openFile(path);
                }
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "Notes",
    .name = "Notes",
    .icon = TT_ASSETS_APP_ICON_NOTES,
    .createApp = create<NotesApp>
};

} // namespace tt::app::notes