#include "RgbDisplay.h"

#include <Tactility/Log.h>

#include <esp_err.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lvgl_port.h>

#define TAG "RgbDisplay"

bool RgbDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    if (esp_lcd_new_rgb_panel(&configuration->panelConfig, &panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    if (esp_lcd_panel_reset(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return false;
    }

    if (esp_lcd_panel_init(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panelHandle, configuration->swapXY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to swap XY");
        return false;
    }

    if (esp_lcd_panel_mirror(panelHandle, configuration->mirrorX, configuration->mirrorY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_invert_color(panelHandle, configuration->invertColor) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to invert");
        return false;
    }

    auto horizontal_resolution = configuration->panelConfig.timings.h_res;
    auto vertical_resolution = configuration->panelConfig.timings.v_res;

    uint32_t buffer_size;
    if (configuration->bufferConfiguration.size == 0) {
        buffer_size = horizontal_resolution * vertical_resolution / 15;
    } else {
        buffer_size = configuration->bufferConfiguration.size;
    }

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = configuration->bufferConfiguration.doubleBuffer,
        .trans_size = 0,
        .hres = horizontal_resolution,
        .vres = vertical_resolution,
        .monochrome = false,
        .rotation = {
            .swap_xy = configuration->swapXY,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY,
        },
        .color_format = configuration->colorFormat,
        .flags = {
            .buff_dma = !configuration->bufferConfiguration.useSpi,
            .buff_spiram = configuration->bufferConfiguration.useSpi,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    const lvgl_port_display_rgb_cfg_t rgb_config = {
        .flags = {
            .bb_mode = configuration->bufferConfiguration.bounceBufferMode,
            .avoid_tearing = configuration->bufferConfiguration.avoidTearing
        }
    };

    displayHandle = lvgl_port_add_disp_rgb(&display_config, &rgb_config);
    TT_LOG_I(TAG, "Finished");

    return displayHandle != nullptr;
}

bool RgbDisplay::stop() {
    assert(displayHandle != nullptr);

    lvgl_port_remove_disp(displayHandle);

    if (esp_lcd_panel_del(panelHandle) != ESP_OK) {
        return false;
    }

    if (esp_lcd_panel_io_del(ioHandle) != ESP_OK) {
        return false;
    }

    displayHandle = nullptr;
    return true;
}
