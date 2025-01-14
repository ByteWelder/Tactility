#pragma once

#include "./View.h"
#include "./State.h"

#include "app/AppManifest.h"

#include <lvgl.h>
#include <dirent.h>
#include <memory>

namespace tt::app::files {

class Files {
    std::unique_ptr<View> view;
    std::shared_ptr<State> state;

public:
    Files() {
        state = std::make_shared<State>();
        view = std::make_unique<View>(state);
    }

    void onShow(lv_obj_t* parent) {
        view->init(parent);
    }

    void onResult(Result result, const Bundle& bundle) {
        view->onResult(result, bundle);
    }
};


} // namespace
