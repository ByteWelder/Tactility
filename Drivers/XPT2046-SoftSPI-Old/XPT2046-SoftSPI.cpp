#include "XPT2046-SoftSPI.h"
#include <esp_log.h>
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";

std::unique_ptr<XPT2046_SoftSPI> XPT2046_SoftSPI::create(const Config& config) {
    // Directly use config fields to create the driver
    auto driver = std::unique_ptr<XPT2046_SoftSPI>(new XPT2046_SoftSPI(config));
    if (!driver) {
        ESP_LOGE(TAG, "Failed to create XPT2046 driver");
        return nullptr;
    }
    ESP_LOGI(TAG, "SoftSPI timings: delay_us=%" PRIu32 ", post_command_delay_us=%" PRIu32, config.spi_delay_us, config.spi_post_command_delay_us);
    ESP_LOGI(TAG, "XPT2046 SoftSPI driver created");
    return driver;
}
