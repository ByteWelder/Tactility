#include "config.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "Log.h"

#define TAG "tdeck_display"

bool tdeck_backlight_init() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = TDECK_LCD_BACKLIGHT_LEDC_MODE,
        .duty_resolution = TDECK_LCD_BACKLIGHT_LEDC_DUTY_RES,
        .timer_num = TDECK_LCD_BACKLIGHT_LEDC_TIMER,
        .freq_hz = TDECK_LCD_BACKLIGHT_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight led timer config failed");
        return false;
    }

    return true;
}

void tdeck_backlight_set(uint8_t duty) {
    ledc_channel_config_t ledc_channel = {
        .gpio_num = TDECK_LCD_PIN_BACKLIGHT,
        .speed_mode = TDECK_LCD_BACKLIGHT_LEDC_MODE,
        .channel = TDECK_LCD_BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = TDECK_LCD_BACKLIGHT_LEDC_TIMER,
        .duty = duty,
        .hpoint = 0
    };

    // Setting the config in the timer init and then calling ledc_set_duty() doesn't work when
    // the app is running. For an unknown reason we have to call this config method every time:
    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
    }
}

lv_display_t* tdeck_display_init() {
    const esp_lcd_panel_io_spi_config_t panel_io_config = {
        .cs_gpio_num = TDECK_LCD_PIN_CS,
        .dc_gpio_num = TDECK_LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = TDECK_LCD_SPI_FREQUENCY,
        .trans_queue_depth = 10,
        .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
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
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TDECK_LCD_SPI_HOST, &panel_io_config, &io_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to create panel IO");
        return nullptr;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = TDECK_LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = 0
        },
        .vendor_config = nullptr
    };

    esp_lcd_panel_handle_t panel_handle;
    if (esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to create panel");
        return nullptr;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to reset panel");
        return nullptr;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return nullptr;
    }

    if (esp_lcd_panel_invert_color(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return nullptr;
    }

    if (esp_lcd_panel_swap_xy(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return nullptr;
    }

    if (esp_lcd_panel_mirror(panel_handle, true, false) != ESP_OK) {
        TT_LOG_E(TAG, "failed to init panel");
        return nullptr;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        TT_LOG_E(TAG, "failed to turn display on");
        return nullptr;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = TDECK_LCD_HORIZONTAL_RESOLUTION * TDECK_LCD_DRAW_BUFFER_HEIGHT * (TDECK_LCD_BITS_PER_PIXEL / 8),
        .double_buffer = true, // Disable to free up SPIRAM
        .hres = TDECK_LCD_HORIZONTAL_RESOLUTION,
        .vres = TDECK_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
            .swap_bytes = true
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}
