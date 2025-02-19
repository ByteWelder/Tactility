#include <Tactility/Log.h>
#include <driver/ledc.h>
#include <driver/gpio.h>

#define TAG "pwm_backlight"

namespace driver::pwmbacklight {

static bool isBacklightInitialized = false;
static gpio_num_t backlightPin = GPIO_NUM_NC;

bool init(gpio_num_t pin) {
    backlightPin = pin;

    TT_LOG_I(TAG, "Init");
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };

    if (ledc_timer_config(&ledc_timer) != ESP_OK) {
        TT_LOG_E(TAG, "Timer config failed");
        return false;
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = backlightPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0
        }
    };

    if (ledc_channel_config(&ledc_channel) != ESP_OK) {
        TT_LOG_E(TAG, "Channel config failed");
    }

    isBacklightInitialized = true;

    return true;
}

bool setBacklightDuty(uint8_t duty) {
    if (!isBacklightInitialized) {
        TT_LOG_E(TAG, "Not initialized");
        return false;
    }
    return ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty) == ESP_OK &&
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) == ESP_OK;
}

}
