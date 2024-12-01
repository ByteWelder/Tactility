#include <driver/gpio.h>
#include "TactilityCore.h"

#define TAG "tdeck"

// Power on
#define TDECK_POWERON_GPIO GPIO_NUM_10

static bool tdeck_power_on() {
    gpio_config_t device_power_signal_config = {
        .pin_bit_mask = BIT64(TDECK_POWERON_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&device_power_signal_config) != ESP_OK) {
        return false;
    }

    if (gpio_set_level(TDECK_POWERON_GPIO, 1) != ESP_OK) {
        return false;
    }

    return true;
}

bool tdeck_init_power() {
    ESP_LOGI(TAG, "Power on");
    if (!tdeck_power_on()) {
        TT_LOG_E(TAG, "Power on failed");
        return false;
    }

    return true;
}
