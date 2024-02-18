#include "config.h"
#include "tactility_core.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#define TAG "twodotfour_ili9341"

// Dipslay backlight (PWM)
#define TWODOTFOUR_LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define TWODOTFOUR_LCD_BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define TWODOTFOUR_LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define TWODOTFOUR_LCD_BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define TWODOTFOUR_LCD_BACKLIGHT_LEDC_FREQUENCY (1000)

bool twodotfour_backlight_init() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = TWODOTFOUR_LCD_BACKLIGHT_LEDC_MODE,
        .timer_num = TWODOTFOUR_LCD_BACKLIGHT_LEDC_TIMER,
        .duty_resolution = TWODOTFOUR_LCD_BACKLIGHT_LEDC_DUTY_RES,
        .freq_hz = TWODOTFOUR_LCD_BACKLIGHT_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight led timer config failed");
        return false;
    }

    return true;
}

void twodotfour_backlight_set(uint8_t duty) {
    ledc_channel_config_t ledc_channel = {
        .speed_mode = TWODOTFOUR_LCD_BACKLIGHT_LEDC_MODE,
        .channel = TWODOTFOUR_LCD_BACKLIGHT_LEDC_CHANNEL,
        .timer_sel = TWODOTFOUR_LCD_BACKLIGHT_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = TWODOTFOUR_LCD_PIN_BACKLIGHT,
        .duty = duty,
        .hpoint = 0
    };

    // Setting the config in the timer init and then calling ledc_set_duty() doesn't work when
    // the app is running. For an unknown reason we have to call this config method every time:
    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
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
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = true
        }
    };

    lv_display_t* display = lvgl_port_add_disp(&disp_cfg);

    return display;
}
