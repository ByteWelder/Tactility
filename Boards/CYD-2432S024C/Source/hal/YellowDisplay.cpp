#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "YellowTouch.h"
#include "Log.h"

#include <TactilityCore.h>
#include <esp_lcd_panel_commands.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#define TAG "yellow_display"

static bool isBacklightInitialized = false;

static bool initBacklight() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = TWODOTFOUR_LCD_BACKLIGHT_LEDC_MODE,
        .duty_resolution = TWODOTFOUR_LCD_BACKLIGHT_LEDC_DUTY_RES,
        .timer_num = TWODOTFOUR_LCD_BACKLIGHT_LEDC_TIMER,
        .freq_hz = TWODOTFOUR_LCD_BACKLIGHT_LEDC_FREQUENCY,
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
        .gpio_num = TWODOTFOUR_LCD_PIN_BACKLIGHT,
        .speed_mode = TWODOTFOUR_LCD_BACKLIGHT_LEDC_MODE,
        .channel = TWODOTFOUR_LCD_BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = TWODOTFOUR_LCD_BACKLIGHT_LEDC_TIMER,
        .duty = duty,
        .hpoint = 0,
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

bool YellowDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    const esp_lcd_panel_io_spi_config_t panel_io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        TWODOTFOUR_LCD_PIN_CS,
        TWODOTFOUR_LCD_PIN_DC,
        nullptr,
        nullptr
    );

    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TWODOTFOUR_LCD_SPI_HOST, &panel_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = TWODOTFOUR_LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = false
        },
        .vendor_config = nullptr
    };

    if (esp_lcd_new_panel_ili9341(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create ili9341");
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

    if (esp_lcd_panel_mirror(panelHandle, true, false) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = TWODOTFOUR_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = false,
        .trans_size = 0,
        .hres = TWODOTFOUR_LCD_HORIZONTAL_RESOLUTION,
        .vres = TWODOTFOUR_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = true,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    displayHandle = lvgl_port_add_disp(&disp_cfg);
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

void YellowDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isBacklightInitialized) {
        tt_check(initBacklight());
        isBacklightInitialized = true;
    }

    if (!setBacklight(backlightDuty)) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
    }
}

/**
 * Note:
 * The datasheet implies this should work, but it doesn't:
 * https://www.digikey.com/htmldatasheets/production/1640716/0/0/1/ILI9341-Datasheet.pdf
 *
 * This repo claims it only has 1 curve:
 * https://github.com/brucemack/hello-ili9341
 *
 * I'm leaving it in as I'm not sure if it's just my hardware that's problematic.
 */
void YellowDisplay::setGammaCurve(uint8_t index) {
    uint8_t gamma_curve;
    switch (index) {
        case 0:
            gamma_curve = 0x01;
            break;
        case 1:
            gamma_curve = 0x04;
            break;
        case 2:
            gamma_curve = 0x02;
            break;
        case 3:
            gamma_curve = 0x08;
            break;
        default:
            return;
    }
    const uint8_t param[] = {
        gamma_curve
    };

    if (esp_lcd_panel_io_tx_param(ioHandle , LCD_CMD_GAMSET, param, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma");
    }
}

tt::hal::Touch* _Nullable YellowDisplay::createTouch() {
    return static_cast<tt::hal::Touch*>(new YellowTouch());
}

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new YellowDisplay());
}
