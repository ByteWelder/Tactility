#include "XPT2046-SoftSPI.h"
#include <esp_log.h>
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";

std::unique_ptr<XPT2046_SoftSPI> XPT2046_SoftSPI::create(const Config& config) {
    // Directly use config fields. If lower-level structs are needed, initialize them here using config's fields.

    // Example: pass config to the driver constructor
    auto driver = std::unique_ptr<XPT2046_SoftSPI>(new XPT2046_SoftSPI(config));
    if (!driver) {
        ESP_LOGE(TAG, "Failed to create XPT2046 driver");
        return nullptr;
    }
    ESP_LOGI(TAG, "SoftSPI timings: delay_us=%" PRIu32 ", post_command_delay_us=%" PRIu32, config.spi_delay_us, config.spi_post_command_delay_us);
    ESP_LOGI(TAG, "XPT2046 SoftSPI driver created");
    return driver;
}
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

    XPT2046_SoftSPI::Config driver_config = config;
    ESP_LOGI(TAG, "SoftSPI timings: delay_us=%" PRIu32 ", post_command_delay_us=%" PRIu32, driver_config.spi_delay_us, driver_config.spi_post_command_delay_us);

    auto driver = std::unique_ptr<XPT2046_SoftSPI>(new XPT2046_SoftSPI(driver_config));
    if (!driver) {
        ESP_LOGE(TAG, "Failed to create XPT2046 driver");
        return nullptr;
    }
    ESP_LOGI(TAG, "XPT2046 SoftSPI driver created");
    return driver;
}
