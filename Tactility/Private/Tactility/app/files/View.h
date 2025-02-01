#pragma once

#include "./State.h"

#include "Tactility/app/AppManifest.h"

#include <lvgl.h>
#include <memory>

namespace tt::app::files {

class View {
    std::shared_ptr<State> state;

    lv_obj_t* dir_entry_list = nullptr;
    lv_obj_t* action_list = nullptr;
    lv_obj_t* navigate_up_button = nullptr;

    void showActionsForDirectory();
    void showActionsForFile();

    void viewFile(const std::string&path, const std::string&filename);
    void createDirEntryWidget(lv_obj_t* parent, struct dirent& dir_entry);
    void onNavigate();

public:

    explicit View(const std::shared_ptr<State>& state) : state(state) {}

    void init(lv_obj_t* parent);
    void update();

    void onNavigateUpPressed();
    void onDirEntryPressed(uint32_t index);
    void onDirEntryLongPressed(int32_t index);
    void onRenamePressed();
    void onDeletePressed();
    void onDirEntryListScrollBegin();
    void onResult(Result result, std::unique_ptr<Bundle> bundle);
};

}
