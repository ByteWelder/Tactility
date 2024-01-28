#include "config.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "log.h"

#define TAG "tdeck_display"

void tdeck_enable_backlight() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LCD_BACKLIGHT_LEDC_MODE,
        .timer_num = LCD_BACKLIGHT_LEDC_TIMER,
        .duty_resolution = LCD_BACKLIGHT_LEDC_DUTY_RES,
        .freq_hz = LCD_BACKLIGHT_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LCD_BACKLIGHT_LEDC_MODE,
        .channel = LCD_BACKLIGHT_LEDC_CHANNEL,
        .timer_sel = LCD_BACKLIGHT_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LCD_BACKLIGHT_LEDC_OUTPUT_IO,
        .duty = 0, // Set duty to 0%
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, LCD_BACKLIGHT_LEDC_DUTY));
}

lv_disp_t* tdeck_display_init() {
    const esp_lcd_panel_io_spi_config_t panel_io_config = {
        .cs_gpio_num = LCD_PIN_CS,
        .dc_gpio_num = LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_FREQUENCY,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode = 0,
            .quad_mode = 0,
            .sio_mode = 1,
            .lsb_first = 0,
            .cs_high_active = 0,
        }
    };

    esp_lcd_panel_io_handle_t io_handle;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &panel_io_config, &io_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to create panel IO");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = 0
        },
        .vendor_config = NULL
    };

    esp_lcd_panel_handle_t panel_handle;
    if (esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to create panel");
        return false;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to reset panel");
        return false;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_invert_color(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_mirror(panel_handle, true, false) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to turn display on");
        return false;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_HORIZONTAL_RESOLUTION * LCD_DRAW_BUFFER_HEIGHT * (LCD_BITS_PER_PIXEL / 8),
        .double_buffer = true, // Disable to free up SPIRAM
        .hres = LCD_HORIZONTAL_RESOLUTION,
        .vres = LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        }
    };

    lv_disp_t* display = lvgl_port_add_disp(&disp_cfg);

    return display;
}
