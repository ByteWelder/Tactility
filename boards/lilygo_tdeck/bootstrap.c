#include "esp_log.h"
#include "driver/gpio.h"
#include "kernel.h"

#define TAG "lilygo_tdeck_bootstrap"
#define TDECK_PERI_POWERON GPIO_NUM_10

static void tdeck_power_on() {
    ESP_LOGI(TAG, "power on");
    gpio_config_t device_power_signal_config = {
        .pin_bit_mask = BIT64(TDECK_PERI_POWERON),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&device_power_signal_config);
    gpio_set_level(TDECK_PERI_POWERON, 1);
}

void lilygo_tdeck_bootstrap() {
    tdeck_power_on();
    // Give keyboard's ESP time to boot
    // It uses I2C and seems to interfere with the touch driver
    tt_delay_ms(500);
}
