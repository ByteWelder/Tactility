#include "PwmBacklight.h"
#include "Tactility/kernel/SystemEvents.h"
#include "Tactility/service/gps/GpsService.h"

#include <Tactility/TactilityCore.h>
#include <Tactility/hal/gps/GpsConfiguration.h>

#include <driver/gpio.h>

#include <Bq27220.h>
#include <Tca8418.h>

#define TAG "tpager"

// Power on
#define TDECK_POWERON_GPIO GPIO_NUM_10

std::shared_ptr<Bq27220> bq27220;
std::shared_ptr<Tca8418> tca8418;

bool tpagerInit() {
    ESP_LOGI(TAG, LOG_MESSAGE_POWER_ON_START);

    /* 32 Khz and higher gives an issue where the screen starts dimming again above 80% brightness
     * when moving the brightness slider rapidly from a lower setting to 100%.
     * This is not a slider bug (data was debug-traced) */
    if (!driver::pwmbacklight::init(GPIO_NUM_42, 30000)) {
        TT_LOG_E(TAG, "Backlight init failed");
        return false;
    }

    bq27220 = std::make_shared<Bq27220>(I2C_NUM_0);
    tt::hal::registerDevice(bq27220);

    tca8418 = std::make_shared<Tca8418>(I2C_NUM_0);
    tt::hal::registerDevice(tca8418);

    tt::kernel::subscribeSystemEvent(tt::kernel::SystemEvent::BootSplash, [](tt::kernel::SystemEvent event) {
        bq27220->configureCapacity(1500, 1500);

        auto gps_service = tt::service::gps::findGpsService();
        if (gps_service != nullptr) {
            std::vector<tt::hal::gps::GpsConfiguration> gps_configurations;
            gps_service->getGpsConfigurations(gps_configurations);
            if (gps_configurations.empty()) {
                if (gps_service->addGpsConfiguration(tt::hal::gps::GpsConfiguration {.uartName = "Grove", .baudRate = 38400, .model = tt::hal::gps::GpsModel::UBLOX10})) {
                    TT_LOG_I(TAG, "Configured internal GPS");
                } else {
                    TT_LOG_E(TAG, "Failed to configure internal GPS");
                }
            }
        }
    });
    return true;
}
