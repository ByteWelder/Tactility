#include <Tactility/app/AppManifest.h>
#include <Tactility/file/File.h>
#include <Tactility/lvgl/Keyboard.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>

#include <dirent.h>
#include <fstream>

namespace tt::app::notes {

constexpr const char* TAG = "Notes";

class NotesApp : public App {
    AppContext* appContext = nullptr;

    lv_obj_t* uiCurrentFileName;
    lv_obj_t* uiDropDownMenu;
    lv_obj_t* uiFileList;
    lv_obj_t* uiFileListCloseBtn;
    lv_obj_t* uiNoteText;
    lv_obj_t* uiSaveDialog;
    lv_obj_t* uiSaveDialogFileName;
    lv_obj_t* uiSaveDialogSaveBtn;
    lv_obj_t* uiSaveDialogCancelBtn;
    lv_obj_t* uiMessageBox;
    lv_obj_t* uiMessageBoxButtonOk;
    lv_obj_t* uiMessageBoxButtonNo;

    char menuItem[32];
    uint8_t menuIdx = 0;
    std::string fileContents;
    std::string fileName;
    std::string newFileName;
    std::string filePath;

#pragma region Main_Events_Functions

    void appNotesEventCb(lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* obj = lv_event_get_target_obj(e);

        if (code == LV_EVENT_CLICKED) {
            if (obj == uiFileListCloseBtn) {
                lv_obj_add_flag(uiFileList, LV_OBJ_FLAG_HIDDEN);
                lv_obj_del(uiFileList);
            } else if (obj == uiSaveDialogSaveBtn) {
                newFileName = lv_textarea_get_text(uiSaveDialogFileName);
                if (newFileName.length() == 0) {
                    uiMessageBoxShow(menuItem, "Filename is empty.", false);
                } else {
                    std::string noteText = lv_textarea_get_text(uiNoteText);
                    filePath = appContext->getPaths()->getDataPath(newFileName);

                    if (writeFile(filePath, noteText)) {
                        uiMessageBoxShow(menuItem, "File created successfully!", false);
                        lv_label_set_text(uiCurrentFileName, newFileName.c_str());
                    } else {
                        uiMessageBoxShow(menuItem, "Something went wrong!\nFile creation failed.", false);
                    }
                }
                lv_obj_del(uiSaveDialog);

            } else if (obj == uiMessageBoxButtonOk || obj == uiMessageBoxButtonNo) {
                lv_obj_del(uiMessageBox);
            } else if (obj == uiSaveDialogCancelBtn) {
                lv_obj_del(uiSaveDialog);
            }
        }

        if (code == LV_EVENT_VALUE_CHANGED) {
            if (obj == uiDropDownMenu) {
                lv_dropdown_get_selected_str(obj, menuItem, sizeof(menuItem));
                menuIdx = lv_dropdown_get_selected(obj);
                std::string newContents = lv_textarea_get_text(uiNoteText);
                if (menuIdx == 1) { //Save
                    //Normal Save?
                }
                if (menuIdx == 2) { //Save As...
                    uiSaveFileDialog();
                    return;
                }

                //Not working...more investigation needed.
                //If note contents has changed in currently open file, save it.

                //bool newToSave = newContents != fileContents && newContents.length() != 0;
                //if (newToSave) {
                    //uiMessageBoxShow(menuItem, "Do you want to save it?", true);
                //} else {
                    menuAction();
                //}
            } else {
                lv_obj_t* cont = lv_event_get_current_target_obj(e);
                if (obj == cont) return;
                if (lv_obj_get_child(cont, 1)) {
                    uiSaveFileDialog();
                } else { //Reset
                    lv_textarea_set_text(uiNoteText, "");
                    fileName = "";
                    lv_label_set_text(uiCurrentFileName, "Untitled");
                }
                lv_obj_delete(uiMessageBox);
            }
        }
    }

    void menuAction() {
        switch (menuIdx) {
            case 0: //Reset
                lv_textarea_set_text(uiNoteText, "");
                fileName = "";
                lv_label_set_text(uiCurrentFileName, "Untitled");
                break;
            case 3:
                uiOpenFileDialog();
                break;
        }
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
            lv_obj_add_event_cb(uiMessageBoxButtonOk, [](lv_event_t* e) {
                    auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                    self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);
        }
    }

#pragma endregion Main_Events_Functions

#pragma region Save_Events_Functions

    void uiSaveFileDialog() {
        uiSaveDialog = lv_obj_create(lv_scr_act());
        if (lv_display_get_horizontal_resolution(nullptr) <= 240 || lv_display_get_vertical_resolution(nullptr) <= 240) { //small screens
            lv_obj_set_size(uiSaveDialog, lv_display_get_horizontal_resolution(nullptr), lv_display_get_vertical_resolution(nullptr) - 80);
        } else { //large screens
            lv_obj_set_size(uiSaveDialog, lv_display_get_horizontal_resolution(nullptr), lv_display_get_vertical_resolution(nullptr) - 230);
        }
        lv_obj_align(uiSaveDialog, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(uiSaveDialog, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* uiSaveDialogTitle = lv_label_create(uiSaveDialog);
        lv_label_set_text(uiSaveDialogTitle, menuItem);
        lv_obj_set_size(uiSaveDialogTitle, lv_display_get_horizontal_resolution(nullptr) - 30, 30);
        lv_obj_align(uiSaveDialogTitle, LV_ALIGN_TOP_MID, 0, 0);

        uiSaveDialogFileName = lv_textarea_create(uiSaveDialog);
        lv_obj_set_size(uiSaveDialogFileName, lv_display_get_horizontal_resolution(nullptr) - 30, 40);
        lv_obj_align_to(uiSaveDialogFileName, uiSaveDialogTitle, LV_ALIGN_TOP_MID, 0, 30);
        lv_textarea_set_placeholder_text(uiSaveDialogFileName, "Enter file name...");
        lv_textarea_set_one_line(uiSaveDialogFileName, true);
        lv_obj_add_state(uiSaveDialogFileName, LV_STATE_FOCUSED);

        //Both hardware and software keyboard not auto attaching here for some reason unless the textarea is touched to focus it...
        tt::lvgl::keyboard_add_textarea(uiSaveDialogFileName);

        if (fileName != "" || fileName != "Untitled") {
            lv_textarea_set_text(uiSaveDialogFileName, fileName.c_str());
        } else {
            lv_textarea_set_placeholder_text(uiSaveDialogFileName, "filename?");
        }

        uiSaveDialogSaveBtn = lv_btn_create(uiSaveDialog);
        lv_obj_add_event_cb(uiSaveDialogSaveBtn, [](lv_event_t* e) {
                auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);
        lv_obj_align(uiSaveDialogSaveBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t* btnLabel = lv_label_create(uiSaveDialogSaveBtn);
        lv_label_set_text(btnLabel, "Save");
        lv_obj_center(btnLabel);

        uiSaveDialogCancelBtn = lv_btn_create(uiSaveDialog);
        lv_obj_add_event_cb(uiSaveDialogCancelBtn, [](lv_event_t* e) {
                auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);
        lv_obj_align(uiSaveDialogCancelBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_t* btnLabel2 = lv_label_create(uiSaveDialogCancelBtn);
        lv_label_set_text(btnLabel2, "Cancel");
        lv_obj_center(btnLabel2);
    }

    bool writeFile(std::string path, std::string message) {
        std::ofstream fileStream(path);

        if (!fileStream.is_open()) {
            TT_LOG_E(TAG, "Failed to write file");
            return false;
        }
        if (fileStream.is_open()) {
            fileStream << message;
            TT_LOG_I(TAG, "File written successfully");
            fileStream.close();
            return true;
        }
        return true;
    }

#pragma endregion Save_Events_Functions

#pragma region Open_Events_Functions

    void openFileEventCb(lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* obj = lv_event_get_target_obj(e);

        if (code == LV_EVENT_CLICKED) {
            std::string selectedFile = lv_list_get_btn_text(uiFileList, obj);
            fileName = selectedFile.substr(0, selectedFile.find(" ("));
            std::string filePath = appContext->getPaths()->getDataPath(fileName);
            fileContents = readFile(filePath.c_str());
            lv_textarea_set_text(uiNoteText, fileContents.c_str());
            lv_obj_add_flag(uiFileList, LV_OBJ_FLAG_HIDDEN);
            lv_obj_del(uiFileList);

            lv_label_set_text(uiCurrentFileName, fileName.c_str());
        }
    }

    void uiOpenFileDialog() {
        uiFileList = lv_list_create(lv_scr_act());
        lv_obj_set_size(uiFileList, lv_display_get_horizontal_resolution(nullptr), lv_display_get_vertical_resolution(nullptr));
        lv_obj_align(uiFileList, LV_ALIGN_TOP_MID, 0, 0);
        lv_list_add_text(uiFileList, "Notes");

        uiFileListCloseBtn = lv_btn_create(uiFileList);
        lv_obj_set_size(uiFileListCloseBtn, 36, 36);
        lv_obj_add_flag(uiFileListCloseBtn, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(uiFileListCloseBtn, LV_ALIGN_TOP_RIGHT, 10, 4);
        lv_obj_add_event_cb(uiFileListCloseBtn, [](lv_event_t* e) {
                auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                self->appNotesEventCb(e); }, LV_EVENT_CLICKED, this);

        lv_obj_t* uiFileListCloseLabel = lv_label_create(uiFileListCloseBtn);
        lv_label_set_text(uiFileListCloseLabel, LV_SYMBOL_CLOSE);
        lv_obj_center(uiFileListCloseLabel);

        lv_obj_add_flag(uiFileList, LV_OBJ_FLAG_HIDDEN);

        //TODO: Move this to SD Card?
        std::vector<std::string> noteFileList;
        const std::string& path = appContext->getPaths()->getDataDirectory();
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            TT_LOG_E(TAG, "Failed to open dir %s", path.c_str());
            return;
        }

        struct dirent* current_entry;
        while ((current_entry = readdir(dir)) != nullptr) {
            noteFileList.push_back(current_entry->d_name);
        }
        closedir(dir);

        if (noteFileList.size() == 0) return;

        for (std::vector<std::string>::iterator item = noteFileList.begin(); item != noteFileList.end(); ++item) {
            lv_obj_t* btn = lv_list_add_btn(uiFileList, LV_SYMBOL_FILE, (*item).c_str());
            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                    self->openFileEventCb(e); }, LV_EVENT_CLICKED, this);
        }

        lv_obj_move_foreground(uiFileListCloseBtn);
        lv_obj_remove_flag(uiFileList, LV_OBJ_FLAG_HIDDEN);
    }

    std::string readFile(std::string path) {
        std::ifstream fileStream(path);

        if (!fileStream.is_open()) {
            return "";
        }

        std::string temp = "";
        std::string file_contents;
        while (std::getline(fileStream, temp)) {
            file_contents += temp;
            file_contents.push_back('\n');
        }
        fileStream.close();
        return file_contents;
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
        lv_obj_align(uiDropDownMenu, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(uiDropDownMenu, [](lv_event_t* e) {
                auto *self = static_cast<NotesApp *>(lv_event_get_user_data(e));
                self->appNotesEventCb(e); }, LV_EVENT_VALUE_CHANGED, this);

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
        lv_obj_set_style_bg_color(uiNoteText, lv_color_make(254, 255, 156), LV_PART_MAIN);
        lv_textarea_set_text(uiNoteText, "This is some random notes\nHere's some more notes!\nThis is some random notes\nHere's some more notes!");

        lv_obj_t* footer = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(footer, lv_color_make(206, 206, 206), LV_PART_MAIN);
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
        if (!tt::file::findOrCreateDirectory(context.getPaths()->getDataDirectory(), 0777)) {
            TT_LOG_E(TAG, "Failed to find or create path %s", context.getPaths()->getDataDirectory().c_str());
        }

        tt::lvgl::keyboard_add_textarea(uiNoteText);
    }
};

extern const AppManifest manifest = {
    .id = "Notes",
    .name = "Notes",
    .createApp = create<NotesApp>
};
} // namespace tt::app::notes