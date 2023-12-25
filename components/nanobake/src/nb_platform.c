#include "nb_platform.h"
#include <esp_check.h>
#include "nb_display.h"
#include "nb_touch.h"
#include "nb_internal.h"

#include <esp_lvgl_port.h>

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

    // Add display
    ESP_LOGD(TAG, "lvgl add display");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = platform->display.io_handle,
        .panel_handle = platform->display.display_handle,
        .buffer_size = platform->display.horizontal_resolution * platform->display.draw_buffer_height * (platform->display.bits_per_pixel / 8),
        .double_buffer = 1,
        .hres = platform->display.horizontal_resolution,
        .vres = platform->display.vertical_resolution,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    platform->lvgl.disp = lvgl_port_add_disp(&disp_cfg);

    // Add touch
    if (platform->touch.io_handle != NULL && platform->touch.touch_handle != NULL) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = platform->lvgl.disp,
            .handle = platform->touch.touch_handle,
        };
        platform->lvgl.touch_indev = lvgl_port_add_touch(&touch_cfg);
        ESP_RETURN_ON_FALSE(platform->lvgl.touch_indev != NULL, ESP_FAIL, TAG, "failed to add touch to lvgl");
    }

    return ESP_OK;
}

esp_err_t nb_platform_create(nb_platform_config_t config, nb_platform_t* platform) {
    ESP_LOGI(TAG, "display with driver %s", config.display_driver.name);
    ESP_RETURN_ON_ERROR(
        nb_display_create(config.display_driver, &(platform->display)),
        nbi_tag,
        "display driver init failed"
    );

    ESP_LOGI(TAG, "touch with driver %s", config.touch_driver.name);
    ESP_RETURN_ON_ERROR(
        nb_touch_create(config.touch_driver, &(platform->touch)),
        nbi_tag,
        "touch driver init failed"
    );

    ESP_RETURN_ON_ERROR(
        prv_lvgl_init(platform),
        nbi_tag,
        "lvgl init failed"
    );

    return ESP_OK;
}
