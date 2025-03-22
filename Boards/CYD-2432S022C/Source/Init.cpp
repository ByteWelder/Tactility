#include <driver/gpio.h>
#include "hal/CYD2432S022CConstants.h"
#include "Tactility/app/display/DisplaySettings.h"
#include <esp_log.h>


#define TAG "CYD2432S022C"

// Hardware resolution (fixed)
#define HARDWARE_WIDTH 240
#define HARDWARE_HEIGHT 320

// Buffer height for partial rendering (fixed)
#define BUFFER_HEIGHT 64

// Default orientation if not set in NVS
#define DEFAULT_ORIENTATION LV_DISPLAY_ROTATION_90

bool cyd22_init() {
    ESP_LOGI(TAG, "Running cyd22_init");

    // Step 1: Configure display reset pin
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

    // Step 2: Configure backlight pin (LovyanGFX will handle PWM later)
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_0);  // Backlight pin
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight pin");
        return false;
    }
    if (gpio_set_level(GPIO_NUM_0, 0) != ESP_OK) return false;  // Backlight off initially

    // Step 3: Get Tactility orientation and set default if needed
    lv_display_rotation_t tactility_orientation = tt::app::display::getRotation();
    if (tactility_orientation == LV_DISPLAY_ROTATION_0) {
        ESP_LOGI(TAG, "No orientation set in NVS. Setting default orientation: %d", DEFAULT_ORIENTATION);
        tt::app::display::setRotation(DEFAULT_ORIENTATION);
        tactility_orientation = DEFAULT_ORIENTATION;
    } else {
        ESP_LOGI(TAG, "Tactility orientation from NVS: %d", tactility_orientation);
    }

    // Step 4: Calculate logical resolution and buffer size based on orientation
    uint16_t logical_width = (tactility_orientation == LV_DISPLAY_ROTATION_90 || tactility_orientation == LV_DISPLAY_ROTATION_270)
                             ? HARDWARE_HEIGHT : HARDWARE_WIDTH;  // 320 or 240
    uint16_t logical_height = (tactility_orientation == LV_DISPLAY_ROTATION_90 || tactility_orientation == LV_DISPLAY_ROTATION_270)
                              ? HARDWARE_WIDTH : HARDWARE_HEIGHT;  // 240 or 320
    size_t buffer_width = logical_width;  // Buffer needs to handle the rotated width
    size_t buffer_size = buffer_width * BUFFER_HEIGHT;  // e.g., 320 * 64 = 20480 pixels

    // Step 5: Log expected settings for validation in LovyanDisplay.cpp
    ESP_LOGI(TAG, "Hardware resolution: %dx%d", HARDWARE_WIDTH, HARDWARE_HEIGHT);
    ESP_LOGI(TAG, "Logical resolution (after rotation): %dx%d", logical_width, logical_height);
    ESP_LOGI(TAG, "Expected buffer size: %d pixels (%d bytes) for %dx%d",
             buffer_size, buffer_size * sizeof(uint16_t), buffer_width, BUFFER_HEIGHT);

    return true;
}
