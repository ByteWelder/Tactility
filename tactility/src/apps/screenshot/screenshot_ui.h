#pragma once

#include "app.h"
#include "lvgl.h"

namespace tt::app::screenshot {

typedef struct {
    lv_obj_t* mode_dropdown;
    lv_obj_t* path_textarea;
    lv_obj_t* start_stop_button_label;
    lv_obj_t* timer_wrapper;
    lv_obj_t* delay_textarea;
} ScreenshotUi;

void create_ui(App app, ScreenshotUi* ui, lv_obj_t* parent);

} // namespace
