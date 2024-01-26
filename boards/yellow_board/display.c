#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "hal/lv_hal_disp.h"
#include <esp_lcd_panel_io.h>

#define TAG "2432s024_ili9341"

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIN_SCLK GPIO_NUM_14
#define LCD_PIN_MOSI GPIO_NUM_13
#define LCD_PIN_CS GPIO_NUM_15
#define LCD_PIN_DC GPIO_NUM_2
#define LCD_PIN_BACKLIGHT GPIO_NUM_27

#define LCD_HORIZONTAL_RESOLUTION 240
#define LCD_VERTICAL_RESOLUTION 320
#define LCD_BITS_PER_PIXEL 16
#define LCD_DRAW_BUFFER_HEIGHT (LCD_VERTICAL_RESOLUTION / 10)

lv_disp_t* yellow_board_init_display() {
    ESP_LOGI(TAG, "creating display");

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(LCD_PIN_BACKLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    size_t draw_buffer_size = LCD_HORIZONTAL_RESOLUTION * LCD_DRAW_BUFFER_HEIGHT * (LCD_BITS_PER_PIXEL / 8);
    const spi_bus_config_t bus_config = ILI9341_PANEL_BUS_SPI_CONFIG(
        LCD_PIN_SCLK,
        LCD_PIN_MOSI,
        draw_buffer_size
    );

    if (spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        ESP_LOGD(TAG, "spi bus init failed");
        return false;
    }

    const esp_lcd_panel_io_spi_config_t panel_io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        LCD_PIN_CS,
        LCD_PIN_DC,
        NULL,
        NULL
    );

    esp_lcd_panel_io_handle_t io_handle;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &panel_io_config, &io_handle) != ESP_OK) {
        ESP_LOGD(TAG, "failed to create panel");
        return false;
    }

    ESP_LOGI(TAG, "install driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };

    esp_lcd_panel_handle_t panel_handle;
    if (esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle) != ESP_OK) {
        ESP_LOGD(TAG, "failed to create ili9341");
        return false;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        ESP_LOGD(TAG, "failed to reset panel");
        return false;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        ESP_LOGD(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_mirror(panel_handle, true, false) != ESP_OK) {
        ESP_LOGD(TAG, "failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        ESP_LOGD(TAG, "failed to turn display on");
        return false;
    }

    if (gpio_set_level(LCD_PIN_BACKLIGHT, 1) != ESP_OK) {
        ESP_LOGD(TAG, "failed to turn backlight on");
        return false;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_HORIZONTAL_RESOLUTION * LCD_DRAW_BUFFER_HEIGHT * (LCD_BITS_PER_PIXEL / 8),
        .double_buffer = false,
        .hres = LCD_HORIZONTAL_RESOLUTION,
        .vres = LCD_VERTICAL_RESOLUTION,
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

    return lvgl_port_add_disp(&disp_cfg);
}
