#include "config.h"
#include "tactility_core.h"

#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#define TAG "core2"

#ifdef __cplusplus
extern "C" {
#endif

lv_disp_t* core2_display_init() {
    TT_LOG_I(TAG, "Display init");

    const esp_lcd_panel_io_spi_config_t panel_io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        CORE2_LCD_PIN_CS,
        CORE2_LCD_PIN_DC,
        nullptr,
        nullptr
    );

    esp_lcd_panel_io_handle_t io_handle;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)CORE2_LCD_SPI_HOST, &panel_io_config, &io_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return nullptr;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = CORE2_LCD_BITS_PER_PIXEL,
    };

    esp_lcd_panel_handle_t panel_handle;
    if (esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create ili9341");
        return nullptr;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return nullptr;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return nullptr;
    }

    if (esp_lcd_panel_invert_color(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to invert panel colours");
        return nullptr;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return nullptr;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = CORE2_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = false,
        .trans_size = CORE2_SPI2_TRANSACTION_LIMIT,
        .hres = CORE2_LCD_HORIZONTAL_RESOLUTION,
        .vres = CORE2_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
            .swap_bytes = true
        }
    };

    lv_display_t* display = lvgl_port_add_disp(&disp_cfg);

    return display;
}

#ifdef __cplusplus
}
#endif
