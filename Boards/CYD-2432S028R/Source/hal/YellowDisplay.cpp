#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "XPT2046_Bitbang.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <string>

static const char* TAG = "YellowDisplay";

// Global to hold reference (only needed if calling stop() later)
static std::unique_ptr<XPT2046_Bitbang> touch;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    ESP_LOGI(TAG, "Creating bitbang SPI touch");
    uint16_t xMinRaw = 300, xMaxRaw = 3800, yMinRaw = 300, yMaxRaw = 3800;

    nvs_handle_t nvs;
    if (nvs_open("touch_cal", NVS_READONLY, &nvs) == ESP_OK) {
        uint16_t cal[4];
        size_t size = sizeof(cal);
        if (nvs_get_blob(nvs, "cal_data", cal, &size) == ESP_OK && size == sizeof(cal)) {
            xMinRaw = cal[0];
            xMaxRaw = cal[1];
            yMinRaw = cal[2];
            yMaxRaw = cal[3];
            ESP_LOGI(TAG, "Loaded NVS calibration: xMinRaw=%u, xMaxRaw=%u, yMinRaw=%u, yMaxRaw=%u",
                     xMinRaw, xMaxRaw, yMinRaw, yMaxRaw);
        } else {
            ESP_LOGW(TAG, "No valid NVS calibration, using defaults: xMinRaw=%u, xMaxRaw=%u, yMinRaw=%u, yMaxRaw=%u",
                     xMinRaw, xMaxRaw, yMinRaw, yMaxRaw);
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "NVS open failed, using default calibration");
    }

    // Create bitbang config object
    auto config = std::make_unique<XPT2046_Bitbang::Configuration>(
        CYD_TOUCH_MOSI_PIN,
        CYD_TOUCH_MISO_PIN,
        CYD_TOUCH_SCK_PIN,
        CYD_TOUCH_CS_PIN,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        false,  // swapXY
        false,  // mirrorX
        false   // mirrorY
    );

    // Allocate the driver
    touch = std::make_unique<XPT2046_Bitbang>(std::move(config));

    class TouchAdapter : public tt::hal::touch::TouchDevice {
    public:
        bool start(lv_display_t* disp) override {
            if (!touch->start(disp)) {
                ESP_LOGE(TAG, "Touch driver start failed");
                return false;
            }
            TT_LVGL_LOCK();
            touch->setCalibration(xMinRaw, yMinRaw, xMaxRaw, yMaxRaw);
            TT_LVGL_UNLOCK();
            return true;
        }
        bool stop() override {
            if (touch) {
                touch->stop();
            }
            return true;
        }
        lv_indev_t* getLvglIndev() override {
            return touch ? touch->get_lvgl_indev() : nullptr;
        }
        std::string getName() const override { return "XPT2046 Touch"; }
        std::string getDescription() const override { return "Bitbang XPT2046 Touch Controller"; }
    };

    return std::make_shared<TouchAdapter>();
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch_device = createTouch();
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD_DISPLAY_SPI_HOST,
        CYD_DISPLAY_PIN_CS,
        CYD_DISPLAY_PIN_DC,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        touch_device
    );
    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    configuration->rgbElementOrder = LCD_RGB_ELEMENT_ORDER_RGB;
    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
