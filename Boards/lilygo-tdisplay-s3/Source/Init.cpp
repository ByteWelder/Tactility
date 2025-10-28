#include "PwmBacklight.h"
#include "Tactility/kernel/SystemEvents.h"

#include <Tactility/TactilityCore.h>
#include <Tactility/hal/spi/Spi.h>

#define TAG "tdisplay-s3"

// Power on
#define TDISPLAY_S3_POWERON_GPIO GPIO_NUM_15

static bool powerOn() {
    gpio_config_t power_signal_config = {
        .pin_bit_mask = BIT64(TDISPLAY_S3_POWERON_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&power_signal_config) != ESP_OK) {
        return false;
    }

    if (gpio_set_level(TDISPLAY_S3_POWERON_GPIO, 1) != ESP_OK) {
        return false;
    }

    return true;
}

bool initBoot() {
    ESP_LOGI(TAG, "Powering on the board...");
    if (!powerOn()) {
        ESP_LOGE(TAG, "Failed to power on the board.");
        return false;
    }

    if (!driver::pwmbacklight::init(GPIO_NUM_2, 30000)) {
        ESP_LOGE(TAG, "Failed to initialize backlight.");
        return false;
    }

    return true;
}