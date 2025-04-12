#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "XPT2046-SoftSPI.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>
#include "esp_log.h"
#include "nvs_flash.h"

static const char* TAG = "YellowDisplay";

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    ESP_LOGI(TAG, "Creating software SPI touch");
    // Default calibration (XPT2046 ADC range, per esp_lcd_touch_xpt2046 (should be correct, if not, driver problem))
    uint16_t xMinRaw = 300, xMaxRaw = 3800, yMinRaw = 300, yMaxRaw = 3800;

    // Load from NVS
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
        .cs_pin = CYD_TOUCH_CS_PIN,  // GPIO 33
        .int_pin = CYD_TOUCH_IRQ_PIN,  // GPIO 36
        .miso_pin = CYD_TOUCH_MISO_PIN,  // GPIO 39
        .mosi_pin = CYD_TOUCH_MOSI_PIN,  // GPIO 32
        .sck_pin = CYD_TOUCH_SCK_PIN,  // GPIO 25
        .x_max = CYD_DISPLAY_HORIZONTAL_RESOLUTION,  // 240
        .y_max = CYD_DISPLAY_VERTICAL_RESOLUTION,  // 320
        .swap_xy = false,
        .mirror_x = false,
        .mirror_y = false,
        .x_min_raw = xMinRaw,
        .x_max_raw = xMaxRaw,
        .y_min_raw = yMinRaw,
        .y_max_raw = yMaxRaw
    };
    auto driver = XPT2046_SoftSPI_Wrapper::create(config);
    class TouchAdapter : public tt::hal::touch::TouchDevice {
    public:
        TouchAdapter(std::unique_ptr<XPT2046_SoftSPI_Wrapper> driver) : driver_(std::move(driver)) {}
        bool init() override { return true; }
        bool start(lv_display_t* disp) override {
            lv_indev_t* indev = driver_->get_lvgl_indev();
            lv_indev_set_display(indev, disp);
            return true;
        }
        bool stop() override { return true; }
    private:
        std::unique_ptr<XPT2046_SoftSPI_Wrapper> driver_;
    };
    return std::make_shared<TouchAdapter>(std::move(driver));
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
