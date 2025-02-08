#include "YellowDisplay.h"
#include "Ili934xDisplay.h"
#include "YellowDisplayConstants.h"
#include "YellowTouch.h"

#include <Tactility/Log.h>
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_commands.h>

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lvgl_port.h>

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

void setBacklightDuty(uint8_t backlightDuty) {
    if (!isBacklightInitialized) {
        tt_check(initBacklight());
        isBacklightInitialized = true;
    }

    if (!setBacklight(backlightDuty)) {
        TT_LOG_E(TAG, "Failed to configure display backlight");
    }
}


std::shared_ptr<tt::hal::Display> createDisplay() {

    auto touch = std::make_shared<YellowTouch>();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        TWODOTFOUR_LCD_SPI_HOST,
        TWODOTFOUR_LCD_PIN_CS,
        TWODOTFOUR_LCD_PIN_DC,
        TWODOTFOUR_LCD_HORIZONTAL_RESOLUTION,
        TWODOTFOUR_LCD_VERTICAL_RESOLUTION,
        touch
    );

    configuration->mirrorX = true;
    configuration->invertColor = false;
    configuration->backlightDutyFunction = ::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
