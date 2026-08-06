// SPDX-License-Identifier: Apache-2.0
#include <lvgl_window_manager/module.h>
#include <lvgl_window_manager/window_manager.h>

#include <tactility/error.h>
#include <tactility/module.h>

extern "C" {

Module lvgl_window_manager_module = {
    .name = "lvgl-window-manager",
    .start = window_manager_start,
    .stop = window_manager_stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr
};

}
