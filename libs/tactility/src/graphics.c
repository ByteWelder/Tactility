#include "check.h"
#include "esp_lvgl_port.h"
#include "graphics_i.h"
#include "lvgl.h"

#define TAG "lvgl"

Lvgl tt_graphics_init(Hardware _Nonnull* hardware) {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    tt_check(lvgl_port_init(&lvgl_cfg) == ESP_OK, "lvgl port init failed");
    DisplayDevice _Nonnull* display = hardware->display;

    // Add display
    ESP_LOGD(TAG, "lvgl add display");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = display->io_handle,
        .panel_handle = display->display_handle,
        .buffer_size = display->horizontal_resolution * display->draw_buffer_height * (display->bits_per_pixel / 8),
        .double_buffer = display->double_buffering,
        .hres = display->horizontal_resolution,
        .vres = display->vertical_resolution,
        .monochrome = display->monochrome,
        .rotation = {
            .swap_xy = display->swap_xy,
            .mirror_x = display->mirror_x,
            .mirror_y = display->mirror_y,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    lv_disp_t _Nonnull* disp = lvgl_port_add_disp(&disp_cfg);
    tt_check(disp != NULL, "failed to add display");

    lv_indev_t _Nullable* touch_indev = NULL;

    // Add touch
    if (hardware->touch != NULL) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = disp,
            .handle = hardware->touch->touch_handle,
        };
        touch_indev = lvgl_port_add_touch(&touch_cfg);
        tt_check(touch_indev != NULL, "failed to add touch to lvgl");
    }

    return (Lvgl) {
        .disp = disp,
        .touch_indev = touch_indev
    };
}
