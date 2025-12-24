#include "PwmBacklight.h"
#include "Tactility/kernel/SystemEvents.h"
#include "Tactility/service/gps/GpsService.h"

#include <Tactility/TactilityCore.h>
#include <Tactility/hal/gps/GpsConfiguration.h>
#include <Tactility/settings/KeyboardSettings.h>

#include "devices/KeyboardBacklight.h"
#include "devices/TrackballDevice.h"
#include <KeyboardBacklight.h>
#include <Trackball.h>

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

static void initI2cDevices() {
    // Defer I2C device startup to avoid heap corruption during early boot
    // Use a one-shot FreeRTOS timer to delay initialization
    static TimerHandle_t initTimer = xTimerCreate(
        "I2CInit",
        pdMS_TO_TICKS(500), // 500ms delay
        pdFALSE, // One-shot
        nullptr,
        [](TimerHandle_t timer) {
            TT_LOG_I(TAG, "Starting deferred I2C devices");
            
            // Start keyboard backlight device
            auto kbBacklight = tt::hal::findDevice("Keyboard Backlight");
            if (kbBacklight) {
                TT_LOG_I(TAG, "%s starting", kbBacklight->getName().c_str());
                auto kbDevice = std::static_pointer_cast<KeyboardBacklightDevice>(kbBacklight);
                if (kbDevice->start()) {
                    TT_LOG_I(TAG, "%s started", kbBacklight->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "%s start failed", kbBacklight->getName().c_str());
                }
            }
            
            // Small delay between I2C device inits to avoid concurrent transactions
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Start trackball device
            auto trackball = tt::hal::findDevice("Trackball");
            if (trackball) {
                TT_LOG_I(TAG, "%s starting", trackball->getName().c_str());
                auto tbDevice = std::static_pointer_cast<TrackballDevice>(trackball);
                if (tbDevice->start()) {
                    TT_LOG_I(TAG, "%s started", trackball->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "%s start failed", trackball->getName().c_str());
                }
            }
            
            TT_LOG_I(TAG, "Deferred I2C devices completed");
        }
    );
    
    if (initTimer != nullptr) {
        xTimerStart(initTimer, 0);
    }
}

static bool keyboardInit() {
    auto kbSettings = tt::settings::keyboard::loadOrGetDefault();

    auto kbBacklight = tt::hal::findDevice("Keyboard Backlight");
    bool result = driver::keyboardbacklight::setBrightness(kbSettings.backlightEnabled ? kbSettings.backlightBrightness : 0);

    if (!result) {
        TT_LOG_W(TAG, "Failed to set keyboard backlight brightness");
    }

    driver::trackball::setEnabled(kbSettings.trackballEnabled);

    return true;
}

bool initBoot() {
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

    tt::kernel::subscribeSystemEvent(tt::kernel::SystemEvent::BootSplash, [](tt::kernel::SystemEvent event) {
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

    initI2cDevices();
    keyboardInit();
    return true;
}
