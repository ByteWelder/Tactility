#pragma once

#include "./State.h"
#include "./FileSelectionPrivate.h"

#include "Tactility/app/AppManifest.h"

#include <lvgl.h>
#include <memory>

namespace tt::app::fileselection {

class View {
    std::shared_ptr<State> state;

    lv_obj_t* dir_entry_list = nullptr;
    lv_obj_t* action_list = nullptr;
    lv_obj_t* navigate_up_button = nullptr;
    std::function<void(std::string path)> on_file_selected;

    void viewFile(const std::string&path, const std::string&filename);
    void createDirEntryWidget(lv_obj_t* parent, dirent& dir_entry);
    void onNavigate();

public:

    explicit View(const std::shared_ptr<State>& state, std::function<void(const std::string& path)> onFileSelected) :
        state(state),
        on_file_selected(std::move(onFileSelected))
    {}

    void init(lv_obj_t* parent, Mode mode);
    void update();

    void onNavigateUpPressed();
    void onDirEntryPressed(uint32_t index);
    void onDirEntryListScrollBegin();
    void onFileSelected(const std::string& path) const {
        on_file_selected(path);
    }
};

}
