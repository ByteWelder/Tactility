#include <driver/gpio.h>
#include "CYD2432S022CConstants.h"

#define TAG "CYD2432S022C"

bool cyd22_init() {
    ESP_LOGI(TAG, "Running cyd22_init");

    // Configure display reset pin
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << CYD_2432S022C_LCD_PIN_RST);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure display reset pin");
        return false;
    }

    // Reset sequence: low, wait, high
    if (gpio_set_level(CYD_2432S022C_LCD_PIN_RST, 0) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(10));
    if (gpio_set_level(CYD_2432S022C_LCD_PIN_RST, 1) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for display to stabilize

    // Optional: Configure backlight pin (LovyanGFX will handle PWM later)
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_0);  // Backlight pin
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight pin");
        return false;
    }
    if (gpio_set_level(GPIO_NUM_0, 0) != ESP_OK) return false;  // Backlight off initially

    // Optional: Reset touch controller if it has a reset pin
    // Add similar GPIO setup if your schematic specifies a touch reset pin

    return true;
}
