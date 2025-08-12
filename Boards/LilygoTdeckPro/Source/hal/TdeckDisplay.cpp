#include "TdeckDisplay.h"
#include "esp_lcd_panel_gdeq.h"
#include "TdeckConstants.h"

#include <Tactility/Log.h>

#include <esp_lvgl_port.h>

#define TAG "EpaperDisplay"

bool TdeckDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    constexpr esp_lcd_panel_io_spi_config_t panel_io_config = {
        .cs_gpio_num = BOARD_EPD_CS,
        .dc_gpio_num = BOARD_EPD_DC,
        .spi_mode = 0,
        .pclk_hz = 4000000,
        .trans_queue_depth = 10,
        .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .flags = {
            .dc_high_on_cmd = 0,
            .dc_low_on_data = 0,
            .dc_low_on_param = 0,
            .octal_mode = 0,
            .quad_mode = 0,
            .sio_mode = 0,
            .lsb_first = 0,
            .cs_high_active = 0
        }
    };

    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &panel_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, // Doesn't matter
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 1,
        .flags = {
            .reset_active_high = false
        },
        .vendor_config = nullptr
    };

    if (esp_lcd_new_panel_gdeq031t10(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
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

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    uint32_t buffer_size = 240 * 320; // Note: Pixel count, not bytes!

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = false,
        .trans_size = 0,
        .hres = 240,
        .vres = 320,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_I1,
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    displayHandle = lvgl_port_add_disp(&disp_cfg);

    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool TdeckDisplay::stop() {
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

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    return std::make_shared<TdeckDisplay>();
}
