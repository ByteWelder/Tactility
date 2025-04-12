#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "SoftXpt2046Touch.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "XPT2046_TouchscreenSOFTSPI.h"  // For CalibrationData

static const char* TAG = "YellowDisplay";

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    ESP_LOGI(TAG, "Creating software SPI touch");
    // Default calibration (fallback)
    uint16_t xMinRaw = 300, xMaxRaw = 3800, yMinRaw = 300, yMaxRaw = 3800;

    // Load from NVS
    nvs_handle_t nvs;
    if (nvs_open("touch_cal", NVS_READONLY, &nvs) == ESP_OK) {
        CalibrationData cal;
        size_t size = sizeof(CalibrationData);
        if (nvs_get_blob(nvs, "cal_data", &cal, &size) == ESP_OK && cal.valid) {
            xMinRaw = -cal.xOffset / cal.xScale;
            xMaxRaw = (CYD_DISPLAY_HORIZONTAL_RESOLUTION - cal.xOffset) / cal.xScale;
            yMinRaw = -cal.yOffset / cal.yScale;
            yMaxRaw = (CYD_DISPLAY_VERTICAL_RESOLUTION - cal.yOffset) / cal.yScale;
            ESP_LOGI(TAG, "Loaded NVS calibration: xMinRaw=%u, xMaxRaw=%u, yMinRaw=%u, yMaxRaw=%u",
                     xMinRaw, xMaxRaw, yMinRaw, yMaxRaw);
        } else {
            ESP_LOGW(TAG, "Using default calibration: xMinRaw=%u, xMaxRaw=%u, yMinRaw=%u, yMaxRaw=%u",
                     xMinRaw, xMaxRaw, yMinRaw, yMaxRaw);
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "NVS open failed, using default calibration");
    }

    auto config = std::make_unique<SoftXpt2046Touch::Configuration>(
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,  // xMax
        CYD_DISPLAY_VERTICAL_RESOLUTION,   // yMax
        false,  // swapXy
        false,  // mirrorX
        false,  // mirrorY
        xMinRaw,  // xMinRaw
        xMaxRaw,  // xMaxRaw
        yMinRaw,  // yMinRaw
        yMaxRaw   // yMaxRaw
    );
    return std::make_shared<SoftXpt2046Touch>(std::move(config));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD_DISPLAY_SPI_HOST,
        CYD_DISPLAY_PIN_CS,
        CYD_DISPLAY_PIN_DC,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        touch
    );
    configuration->mirrorX = true;  // Keep for display
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
