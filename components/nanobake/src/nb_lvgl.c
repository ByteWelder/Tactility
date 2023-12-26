#include "nb_lvgl.h"
#include "nb_hardware.h"
#include <esp_check.h>
#include <check.h>

static const char* TAG = "nb_lvgl";

nb_lvgl_t nb_lvgl_init(nb_hardware_t* platform) {
    nb_lvgl_t lvgl;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    furi_check(lvgl_port_init(&lvgl_cfg) == ESP_OK, "lvgl port init failed");
    nb_display_t _Nonnull* display = platform->display;
    // Add display
    ESP_LOGD(TAG, "lvgl add display");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = display->io_handle,
        .panel_handle = display->display_handle,
        .buffer_size = display->horizontal_resolution * display->draw_buffer_height * (display->bits_per_pixel / 8),
        .double_buffer = 0,
        .hres = display->horizontal_resolution,
        .vres = display->vertical_resolution,
        .monochrome = false,
        /* Rotation values must be same as defined in driver */
        // TODO: expose data from driver
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lvgl.disp = lvgl_port_add_disp(&disp_cfg);

    // Add touch
    if (platform->touch != NULL) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lvgl.disp,
            .handle = platform->touch->touch_handle,
        };
        lvgl.touch_indev = lvgl_port_add_touch(&touch_cfg);
        furi_check(lvgl.touch_indev != NULL, "failed to add touch to lvgl");
    }

    return lvgl;
}
