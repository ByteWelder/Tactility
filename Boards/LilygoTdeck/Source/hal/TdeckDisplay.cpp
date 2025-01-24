#include "TdeckDisplay.h"
#include "TdeckDisplayConstants.h"
#include "TdeckTouch.h"
#include "Log.h"

#include <TactilityCore.h>
#include <esp_lcd_panel_commands.h>

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"

#define TAG "tdeck_display"

static bool isBacklightInitialized = false;

static bool initBacklight() {
    TT_LOG_I(TAG, "Init backlight");
    ledc_timer_config_t ledc_timer = {
        .speed_mode = TDECK_LCD_BACKLIGHT_LEDC_MODE,
        .duty_resolution = TDECK_LCD_BACKLIGHT_LEDC_DUTY_RES,
        .timer_num = TDECK_LCD_BACKLIGHT_LEDC_TIMER,
        .freq_hz = TDECK_LCD_BACKLIGHT_LEDC_FREQUENCY,
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
        .gpio_num = TDECK_LCD_PIN_BACKLIGHT,
        .speed_mode = TDECK_LCD_BACKLIGHT_LEDC_MODE,
        .channel = TDECK_LCD_BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = TDECK_LCD_BACKLIGHT_LEDC_TIMER,
        .duty = duty,
        .hpoint = 0,
        .flags = {
            .output_invert = 0
        }
    };

    // Setting the config in the timer init and then calling ledc_set_duty() doesn't work when
    // the app is running. For an unknown reason we have to call this config method every time:
    return ledc_channel_config(&ledc_channel) == ESP_OK;
}

bool TdeckDisplay::start() {
    TT_LOG_I(TAG, "Starting");

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
            .dc_high_on_cmd = 0,
            .dc_low_on_data = 0,
            .dc_low_on_param = 0,
            .octal_mode = 0,
            .quad_mode = 0,
            .sio_mode = 1,
            .lsb_first = 0,
            .cs_high_active = 0,
        }
    };

    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TDECK_LCD_SPI_HOST, &panel_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel IO");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = TDECK_LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = 0
        },
        .vendor_config = nullptr
    };

    if (esp_lcd_new_panel_st7789(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
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

    if (esp_lcd_panel_invert_color(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to swap XY ");
        return false;
    }

    if (esp_lcd_panel_mirror(panelHandle, true, false) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to mirror panel");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .buffer_size = TDECK_LCD_HORIZONTAL_RESOLUTION * TDECK_LCD_DRAW_BUFFER_HEIGHT * (TDECK_LCD_BITS_PER_PIXEL / 8),
        .double_buffer = true, // Disable to free up SPIRAM
        .trans_size = 0,
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
            .swap_bytes = false
        },
    };

    displayHandle = lvgl_port_add_disp(&disp_cfg);
    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool TdeckDisplay::stop() {
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

void TdeckDisplay::setPowerOn(bool turnOn) {
    if (esp_lcd_panel_disp_on_off(panelHandle, turnOn) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on/off");
    } else {
        poweredOn = turnOn;
    }
}

tt::hal::Touch* _Nullable TdeckDisplay::createTouch() {
    return static_cast<tt::hal::Touch*>(new TdeckTouch());
}

void TdeckDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isBacklightInitialized) {
        tt_check(initBacklight());
        isBacklightInitialized = true;
    }

    if (!setBacklight(backlightDuty)) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
    }
}

void TdeckDisplay::setGammaCurve(uint8_t index) {
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

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new TdeckDisplay());
}
