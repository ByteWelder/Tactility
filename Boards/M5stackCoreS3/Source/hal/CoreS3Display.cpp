#include "CoreS3Display.h"
#include "CoreS3DisplayConstants.h"
#include "Log.h"

#include <TactilityCore.h>
#include <esp_lcd_panel_commands.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "hal/i2c/I2c.h"
#include "CoreS3Constants.h"
#include "CoreS3Touch.h"

#define TAG "cores3_display"

bool CoreS3Display::start() {
    TT_LOG_I(TAG, "Starting");

    const esp_lcd_panel_io_spi_config_t panel_io_config = {
        .cs_gpio_num = CORES3_LCD_PIN_CS,
        .dc_gpio_num = CORES3_LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = 40000000,
        .trans_queue_depth = 10,
        .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
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

    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)CORES3_LCD_SPI_HOST, &panel_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = CORES3_LCD_BITS_PER_PIXEL,
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

    if (esp_lcd_panel_mirror(panelHandle, false, false) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_invert_color(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to invert");
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
        .buffer_size = CORES3_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = true,
        .trans_size = 0,
        .hres = CORES3_LCD_HORIZONTAL_RESOLUTION,
        .vres = CORES3_LCD_VERTICAL_RESOLUTION,
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
            .swap_bytes = true,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    displayHandle = lvgl_port_add_disp(&disp_cfg);
    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool CoreS3Display::stop() {
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
void CoreS3Display::setGammaCurve(uint8_t index) {
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

void CoreS3Display::setBacklightDuty(uint8_t backlightDuty) {
    const uint8_t voltage = 20 + ((8 * backlightDuty) / 255); // [0b00000, 0b11100] - under 20 is too dark
    // TODO: Refactor to use Axp2102 class with https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L42
    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AXP2101_ADDRESS, 0x99, &voltage, 1, 1000)) { // Sets DLD01
        TT_LOG_E(TAG, "Failed to set display backlight voltage");
    }
}

tt::hal::Touch* _Nullable CoreS3Display::createTouch() {
    return static_cast<tt::hal::Touch*>(new CoreS3Touch());
}

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new CoreS3Display());
}
