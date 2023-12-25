#include "nb_platform.h"
#include "nb_display.h"
#include "nb_touch.h"

#include <esp_check.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>

#include <nb_assert.h>

static const char* TAG = "nb_platform";

static esp_err_t prv_lvgl_init(
    nb_platform_t* platform
) {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port init failed");
    nb_display_t _Nonnull* display = platform->display;
    // Add display
    ESP_LOGD(TAG, "lvgl add display");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = display->io_handle,
        .panel_handle = display->display_handle,
        .buffer_size = display->horizontal_resolution * display->draw_buffer_height * (display->bits_per_pixel / 8),
        .double_buffer = 1,
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
    platform->lvgl->disp = lvgl_port_add_disp(&disp_cfg);

    // Add touch
    if (platform->touch != NULL) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = platform->lvgl->disp,
            .handle = platform->touch->touch_handle,
        };
        platform->lvgl->touch_indev = lvgl_port_add_touch(&touch_cfg);
        ESP_RETURN_ON_FALSE(platform->lvgl->touch_indev != NULL, ESP_FAIL, TAG, "failed to add touch to lvgl");
    }

    return ESP_OK;
}

nb_platform_t _Nonnull* nb_platform_create(nb_platform_config_t _Nonnull* config) {
    nb_platform_t* platform = malloc(sizeof(nb_platform_t));

    NB_ASSERT(config->display_driver != NULL, "no display driver configured");
    nb_display_driver_t display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    platform->display = nb_display_create(&display_driver);

    if (config->touch_driver != NULL) {
        nb_touch_driver_t touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        platform->touch = nb_touch_create(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        platform->touch = NULL;
    }

    NB_ASSERT(prv_lvgl_init(platform) == ESP_OK, "failed to init lvgl");

    return platform;
}
