#include "XPT2046-SoftSPI.h"
#include <esp_log.h>

static const char* TAG = "XPT2046_Wrapper";

std::unique_ptr<XPT2046_SoftSPI_Wrapper> XPT2046_SoftSPI_Wrapper::create(const Config& config) {
    XPT2046_SoftSPI::Config driver_config = {
        .cs_pin = config.cs_pin,
        .int_pin = config.int_pin,
        .miso_pin = config.miso_pin,
        .mosi_pin = config.mosi_pin,
        .sck_pin = config.sck_pin,
        .touch_config = {
            .int_gpio_num = config.int_pin,
            .x_max = config.x_max,
            .y_max = config.y_max,
            .swap_xy = config.swap_xy,
            .mirror_x = config.mirror_x,
            .mirror_y = config.mirror_y,
            .x_min_raw = config.x_min_raw,
            .x_max_raw = config.x_max_raw,
            .y_min_raw = config.y_min_raw,
            .y_max_raw = config.y_max_raw,
            .interrupt_callback = nullptr,
            .levels = {.interrupt_level = false}
        }
    };

    auto driver = XPT2046_SoftSPI::create(driver_config);
    if (!driver) {
        ESP_LOGE(TAG, "Failed to create XPT2046 driver");
        return nullptr;
    }

    ESP_LOGI(TAG, "XPT2046 wrapper created");
    return std::make_unique<XPT2046_SoftSPI_Wrapper>(std::move(driver));
}
