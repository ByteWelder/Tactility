#include "config.h"
#include "log.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "hal/lv_hal_disp.h"
#include <esp_lcd_panel_io.h>

#define TAG "twodotfour_ili9341"

static void twodotfour_backlight_on() {
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(TWODOTFOUR_LCD_PIN_BACKLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);

    if (gpio_set_level(TWODOTFOUR_LCD_PIN_BACKLIGHT, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn backlight on");
    }
}

lv_disp_t* twodotfour_display_init() {
    TT_LOG_I(TAG, "Display init");

    const esp_lcd_panel_io_spi_config_t panel_io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        TWODOTFOUR_LCD_PIN_CS,
        TWODOTFOUR_LCD_PIN_DC,
        NULL,
        NULL
    );

    esp_lcd_panel_io_handle_t io_handle;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TWODOTFOUR_LCD_SPI_HOST, &panel_io_config, &io_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = TWODOTFOUR_LCD_BITS_PER_PIXEL,
    };

    esp_lcd_panel_handle_t panel_handle;
    if (esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create ili9341");
        return false;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return false;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return false;
    }

    if (esp_lcd_panel_mirror(panel_handle, true, false) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = TWODOTFOUR_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = false,
        .hres = TWODOTFOUR_LCD_HORIZONTAL_RESOLUTION,
        .vres = TWODOTFOUR_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    lv_disp_t* display = lvgl_port_add_disp(&disp_cfg);

    twodotfour_backlight_on();

    return display;
}
