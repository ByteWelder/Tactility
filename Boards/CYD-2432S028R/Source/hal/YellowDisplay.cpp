#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "XPT2046-SoftSPI.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <string>

static const char* TAG = "YellowDisplay";

std::unique_ptr<XPT2046_SoftSPI_Wrapper> touch;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    ESP_LOGI(TAG, "Creating software SPI touch");
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

    XPT2046_SoftSPI_Wrapper::Config config = {
        .cs_pin = CYD_TOUCH_CS_PIN,
        .int_pin = CYD_TOUCH_IRQ_PIN,
        .miso_pin = CYD_TOUCH_MISO_PIN,
        .mosi_pin = CYD_TOUCH_MOSI_PIN,
        .sck_pin = CYD_TOUCH_SCK_PIN,
        .x_max = CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        .y_max = CYD_DISPLAY_VERTICAL_RESOLUTION,
        .swap_xy = false,
        .mirror_x = false,
        .mirror_y = false,
        .x_min_raw = xMinRaw,
        .x_max_raw = xMaxRaw,
        .y_min_raw = yMinRaw,
        .y_max_raw = yMaxRaw
    };
    touch = XPT2046_SoftSPI_Wrapper::create(config);
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch driver");
        return nullptr;
    }

    class TouchAdapter : public tt::hal::touch::TouchDevice {
    public:
        TouchAdapter(std::unique_ptr<XPT2046_SoftSPI_Wrapper> driver) : driver_(std::move(driver)) {}
        bool start(lv_display_t* disp) override {
            lv_indev_t* indev = driver_->get_lvgl_indev();
            lv_indev_set_display(indev, disp);
            return true;
        }
        bool stop() override { return true; }
        lv_indev_t* getLvglIndev() override { return driver_->get_lvgl_indev(); }
        std::string getName() const override { return "XPT2046 Touch"; }
        std::string getDescription() const override { return "SoftSPI XPT2046 Touch Controller"; }
    private:
        std::unique_ptr<XPT2046_SoftSPI_Wrapper> driver_;
    };
    return std::make_shared<TouchAdapter>(std::move(touch));
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
    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
