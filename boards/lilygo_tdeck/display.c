#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#define TAG "tdeck_display"

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIN_SCLK GPIO_NUM_40
#define LCD_PIN_MOSI GPIO_NUM_41
#define LCD_PIN_MISO GPIO_NUM_38
#define LCD_PIN_CS GPIO_NUM_12
#define LCD_PIN_DC GPIO_NUM_11 // RS
#define LCD_PIN_BACKLIGHT GPIO_NUM_42

#define LCD_SPI_FREQUENCY 40000000
#define LCD_HORIZONTAL_RESOLUTION 320
#define LCD_VERTICAL_RESOLUTION 240
#define LCD_BITS_PER_PIXEL 16
#define LCD_DRAW_BUFFER_HEIGHT (LCD_VERTICAL_RESOLUTION / 10)
#define LCD_SPI_TRANSFER_HEIGHT (LCD_VERTICAL_RESOLUTION / 10)

// Backlight PWM
#define LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define LCD_BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define LCD_BACKLIGHT_LEDC_OUTPUT_IO LCD_PIN_BACKLIGHT
#define LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define LCD_BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define LCD_BACKLIGHT_LEDC_DUTY (191)
#define LCD_BACKLIGHT_LEDC_FREQUENCY (1000)

static void tdeck_backlight() {
    ESP_LOGI(TAG, "enable backlight");

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LCD_BACKLIGHT_LEDC_MODE,
        .timer_num        = LCD_BACKLIGHT_LEDC_TIMER,
        .duty_resolution  = LCD_BACKLIGHT_LEDC_DUTY_RES,
        .freq_hz          = LCD_BACKLIGHT_LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LCD_BACKLIGHT_LEDC_MODE,
        .channel        = LCD_BACKLIGHT_LEDC_CHANNEL,
        .timer_sel      = LCD_BACKLIGHT_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LCD_BACKLIGHT_LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(LCD_BACKLIGHT_LEDC_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, LCD_BACKLIGHT_LEDC_DUTY));
}

lv_disp_t* lilygo_tdeck_init_display() {
    ESP_LOGI(TAG, "creating display");

    int max_transfer_size = LCD_HORIZONTAL_RESOLUTION * LCD_SPI_TRANSFER_HEIGHT * (LCD_BITS_PER_PIXEL / 8);

    spi_bus_config_t bus_config = {
        .sclk_io_num = LCD_PIN_SCLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = max_transfer_size,
    };

    if (spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        ESP_LOGD(TAG, "spi bus init failed");
        return false;
    }

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
        ESP_LOGD(TAG, "failed to create panel IO");
        return false;
    }

    ESP_LOGI(TAG, "install driver");
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
        ESP_LOGD(TAG, "failed to create panel");
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

    if (esp_lcd_panel_invert_color(panel_handle, true) != ESP_OK) {
        ESP_LOGD(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panel_handle, true) != ESP_OK) {
        ESP_LOGD(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_mirror(panel_handle, true, false) != ESP_OK) {
        ESP_LOGD(TAG, "failed to init panel");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panel_handle, true) != ESP_OK) {
        ESP_LOGD(TAG, "failed to turn display on");
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
            .swap_xy = true, // TODO: check if code above is still needed
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        }
    };

    tdeck_backlight();

    return lvgl_port_add_disp(&disp_cfg);
}
