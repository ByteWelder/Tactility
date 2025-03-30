#include "PwmBacklight.h"
#include "Tactility/kernel/SystemEvents.h"
#include "Tactility/service/gps/GpsService.h"

#include <Tactility/TactilityCore.h>
#include <Tactility/hal/gps/GpsConfiguration.h>

#define TAG "tdeck"

// Power on
#define TDECK_POWERON_GPIO GPIO_NUM_10

static bool powerOn() {
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

bool tdeckInit() {
    ESP_LOGI(TAG, LOG_MESSAGE_POWER_ON_START);
    if (!powerOn()) {
        TT_LOG_E(TAG, LOG_MESSAGE_POWER_ON_FAILED);
        return false;
    }

    /* 32 Khz and higher gives an issue where the screen starts dimming again above 80% brightness
     * when moving the brightness slider rapidly from a lower setting to 100%.
     * This is not a slider bug (data was debug-traced) */
    if (!driver::pwmbacklight::init(GPIO_NUM_42, 30000)) {
        TT_LOG_E(TAG, "Backlight init failed");
        return false;
    }

    tt::kernel::systemEventAddListener(tt::kernel::SystemEvent::BootSplash, [](tt::kernel::SystemEvent event){
        auto gps_service = tt::service::gps::findGpsService();
        if (gps_service != nullptr) {
            std::vector<tt::hal::gps::GpsConfiguration> gps_configurations;
            gps_service->getGpsConfigurations(gps_configurations);
            if (gps_configurations.empty()) {
                if (gps_service->addGpsConfiguration(tt::hal::gps::GpsConfiguration {
                    .uartName = "Grove",
                    .baudRate = 38400,
                    .model = tt::hal::gps::GpsModel::UBLOX10
                })) {
                    TT_LOG_I(TAG, "Configured internal GPS");
                } else {
                    TT_LOG_E(TAG, "Failed to configure internal GPS");
                }
            }
        }
    });
    return true;
}
