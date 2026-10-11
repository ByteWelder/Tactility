// SPDX-License-Identifier: Apache-2.0

// Keeps this board's UI clear of the display edges with a fixed inset.
//
// Core Tactility only applies the insets it derives from the display shape for round displays (see
// Tactility/Source/lvgl/Lvgl.cpp), and this rectangular AMOLED reports no shape, so instead of
// changing core source this wraps the window-manager screen-init callback - the same board-local
// link-time wrapping used in lvgl_scaling.cpp.
//
// The padding is applied to the window-manager root widget (the argument the screen-init callback
// receives and builds its chrome under), not just to the app container it returns. That way the
// statusbar, which the callback adds as a sibling of the app container, is inset with the rest of
// the UI instead of sitting on the top edge of the display.
//
// lv_obj_set_style_pad_all() writes the local style prop directly, so it is NOT affected by the
// /8 padding wrapper in lvgl_scaling.cpp (that one only covers lv_style_set_pad_*).

#include <lvgl_window_manager/window_manager.h>

#include <lvgl/lvgl.h>

#include <stdint.h>

namespace {

// Fixed inset, in pixels, applied on all four edges.
constexpr int32_t INSET_PX = 22;

// The callback passed to window_manager_configure(), invoked later to build the window tree.
WindowManagerScreenInitFn original_screen_init = nullptr;

} // namespace

extern "C" {

static lv_obj_t* wrapping_screen_init(lv_obj_t* root_widget) {
    lv_obj_set_style_pad_all(root_widget, INSET_PX, LV_STATE_DEFAULT);
    return original_screen_init != nullptr ? original_screen_init(root_widget) : root_widget;
}

void __real_window_manager_configure(WindowManagerScreenInitFn screen_init);

void __wrap_window_manager_configure(WindowManagerScreenInitFn screen_init) {
    original_screen_init = screen_init;
    __real_window_manager_configure(screen_init != nullptr ? wrapping_screen_init : nullptr);
}

} // extern "C"
