#include "XPT2046-SoftSPI.h"
#include <esp_log.h>

static const char* TAG = "XPT2046_Wrapper";

std::unique_ptr<XPT2046_SoftSPI_Wrapper> XPT2046_SoftSPI_Wrapper::create(const Config& config) {
    esp_lcd_touch_xpt2046_config_t touch_config = {
        .base = {
            .x_max = config.x_max,
            .y_max = config.y_max,
            .rst_gpio_num = GPIO_NUM_NC,
            .int_gpio_num = config.int_pin,
            .flags = {
                .swap_xy = config.swap_xy,
                .mirror_x = config.mirror_x,
                .mirror_y = config.mirror_y,
            },
            .interrupt_callback = nullptr,
            .user_data = nullptr,
        },
        .x_min_raw = config.x_min_raw,
        .x_max_raw = config.x_max_raw,
        .y_min_raw = config.y_min_raw,
        .y_max_raw = config.y_max_raw,
        .swap_xy = config.swap_xy,
        .mirror_x = config.mirror_x,
        .mirror_y = config.mirror_y
    };

    XPT2046_SoftSPI::Config driver_config = {
        .cs_pin = config.cs_pin,
        .int_pin = config.int_pin,
        .miso_pin = config.miso_pin,
        .mosi_pin = config.mosi_pin,
        .sck_pin = config.sck_pin,
        .touch_config = touch_config
    };

    auto driver = XPT2046_SoftSPI::create(driver_config);
    if (!driver) {
        ESP_LOGE(TAG, "Failed to create XPT2046 driver");
        return nullptr;
    }

    driver->get_handle()->config.user_data = &driver_config.touch_config;

    ESP_LOGI(TAG, "XPT2046 wrapper created");
    return std::unique_ptr<XPT2046_SoftSPI_Wrapper>(new XPT2046_SoftSPI_Wrapper(std::move(driver)));
}
