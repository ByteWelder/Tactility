#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"

#include <Gt911Touch.h>
#include <Tactility/Log.h>
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_commands.h>

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_st7701.h>
#include <esp_lvgl_port.h>

#define TAG "yellow_display"

static bool isBacklightInitialized = false;

static bool initBacklight() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = CYD4848S040_LCD_BACKLIGHT_LEDC_MODE,
        .duty_resolution = CYD4848S040_LCD_BACKLIGHT_LEDC_DUTY_RES,
        .timer_num = CYD4848S040_LCD_BACKLIGHT_LEDC_TIMER,
        .freq_hz = CYD4848S040_LCD_BACKLIGHT_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight led timer config failed");
        return false;
    }

    return true;
}

static bool setBacklight(uint8_t duty) {
    ledc_channel_config_t ledc_channel = {
        .gpio_num = CYD4848S040_LCD_PIN_BACKLIGHT,
        .speed_mode = CYD4848S040_LCD_BACKLIGHT_LEDC_MODE,
        .channel = CYD4848S040_LCD_BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = CYD4848S040_LCD_BACKLIGHT_LEDC_TIMER,
        .duty = duty,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = false
        }
    };

    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        TT_LOG_E(TAG, "Backlight init failed");
        return false;
    }

    return true;
}

static const st7701_lcd_init_cmd_t st7701_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]) {0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t[]) {0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]) {0x31, 0x05}, 2, 0},
    {0xCD, (uint8_t[]) {0x00}, 1, 0}, //0x08
    //Positive Voltage Gamma Control
    {0xB0, (uint8_t[]) {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16, 0},
    //Negative Voltage Gamma Control
    {0xB1, (uint8_t[]) {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16, 0},
    //Page1
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]) {0x60}, 1, 0}, //Vop=4.7375v
    {0xB1, (uint8_t[]) {0x32}, 1, 0}, //VCOM=32
    {0xB2, (uint8_t[]) {0x07}, 1, 0}, //VGH=15v
    {0xB3, (uint8_t[]) {0x80}, 1, 0},
    {0xB5, (uint8_t[]) {0x49}, 1, 0}, //VGL=-10.17v
    {0xB7, (uint8_t[]) {0x85}, 1, 0},
    {0xB8, (uint8_t[]) {0x21}, 1, 0}, //AVDD=6.6 & AVCL=-4.6
    {0xC1, (uint8_t[]) {0x78}, 1, 0},
    {0xC2, (uint8_t[]) {0x78}, 1, 0},
    {0xE0, (uint8_t[]) {0x00, 0x1B, 0x02}, 3, 0},
    {0xE1, (uint8_t[]) {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t[]) {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]) {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t[]) {0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]) {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]) {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t[]) {0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]) {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16, 0},
    {0xEB, (uint8_t[]) {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (uint8_t[]) {0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t[]) {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16, 0},
    //VAP & VAN
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xE5, (uint8_t[]) {0xE4}, 1, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x3A, (uint8_t[]) {0x60}, 1, 10}, //0x70 RGB888, 0x60 RGB666, 0x50 RGB565
    {0x11, (uint8_t[]) {0x00}, 0, 120}, //Sleep Out
    {0x29, (uint8_t[]) {0x00}, 0, 0}, //Display On
};

bool YellowDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = CYD4848S040_LCD_PIN_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = GPIO_NUM_48,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = GPIO_NUM_47,
        .io_expander = NULL,
    };
    esp_lcd_panel_io_3wire_spi_config_t panel_io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&panel_io_config, &ioHandle));

    const esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 16000000,
            .h_res = CYD4848S040_LCD_HORIZONTAL_RESOLUTION,
            .v_res = CYD4848S040_LCD_VERTICAL_RESOLUTION,

            .hsync_pulse_width = 10,
            .hsync_back_porch = 10,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 10,
            .vsync_front_porch = 10,
            .flags = {
                .hsync_idle_low = false,
                .vsync_idle_low = false,
                .de_idle_high = false,
                .pclk_active_neg = false,
                .pclk_idle_high = false
            }
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 2,
        .bounce_buffer_size_px = CYD4848S040_LCD_HORIZONTAL_RESOLUTION * 10,
        .sram_trans_align = 8,
        .psram_trans_align = 64,

        .hsync_gpio_num = CYD4848S040_LCD_PIN_HSYNC,
        .vsync_gpio_num = CYD4848S040_LCD_PIN_VSYNC,
        .de_gpio_num = CYD4848S040_LCD_PIN_DE,
        .pclk_gpio_num = CYD4848S040_LCD_PIN_PCLK,
        .disp_gpio_num = CYD4848S040_LCD_PIN_DISP_EN,
        .data_gpio_nums = {
            CYD4848S040_LCD_PIN_DATA0,
            CYD4848S040_LCD_PIN_DATA1,
            CYD4848S040_LCD_PIN_DATA2,
            CYD4848S040_LCD_PIN_DATA3,
            CYD4848S040_LCD_PIN_DATA4,
            CYD4848S040_LCD_PIN_DATA5,
            CYD4848S040_LCD_PIN_DATA6,
            CYD4848S040_LCD_PIN_DATA7,
            CYD4848S040_LCD_PIN_DATA8,
            CYD4848S040_LCD_PIN_DATA9,
            CYD4848S040_LCD_PIN_DATA10,
            CYD4848S040_LCD_PIN_DATA11,
            CYD4848S040_LCD_PIN_DATA12,
            CYD4848S040_LCD_PIN_DATA13,
            CYD4848S040_LCD_PIN_DATA14,
            CYD4848S040_LCD_PIN_DATA15
        },
        .flags = {
            .disp_active_low = false,
            .refresh_on_demand = false,
            .fb_in_psram = true,
            .double_fb = true,
            .no_fb = false,
            .bb_invalidate_cache = true
        }
    };

    st7701_vendor_config_t vendor_config = {
        .init_cmds = st7701_lcd_init_cmds,
        .init_cmds_size = sizeof(st7701_lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
        .rgb_config = &rgb_config,
        .flags = {
            .use_mipi_interface = 0,
            .mirror_by_cmd = 1,
            .auto_del_panel_io = 0,
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
        .flags = {
            .reset_active_high = false,
        },
        .vendor_config = &vendor_config,
    };

    if (esp_lcd_new_panel_st7701(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
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

    if (esp_lcd_panel_invert_color(panelHandle, false) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to invert color");
    }

    esp_lcd_panel_set_gap(panelHandle, 0, 0);

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = CYD4848S040_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = true,
        .trans_size = 0,
        .hres = CYD4848S040_LCD_HORIZONTAL_RESOLUTION,
        .vres = CYD4848S040_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true,
            .avoid_tearing = false
        }
    };

    displayHandle = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool YellowDisplay::stop() {
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

std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable YellowDisplay::createTouch() {
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        480,
        480
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

void YellowDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isBacklightInitialized) {
        tt_check(initBacklight());
        isBacklightInitialized = true;
    }

    if (!setBacklight(backlightDuty)) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
    }
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    return std::make_shared<YellowDisplay>();
}
