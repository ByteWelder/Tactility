#include "Xpt2046SoftSpi.h"

#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <rom/ets_sys.h>

constexpr auto* TAG = "Xpt2046SoftSpi";

constexpr auto RERUN_CALIBRATE = false;
constexpr auto CMD_READ_Y = 0x90; // Try different commands if these don't work
constexpr auto CMD_READ_X = 0xD0; // Alternative: 0x98 for Y, 0xD8 for X

struct Calibration {
    int xMin;
    int xMax;
    int yMin;
    int yMax;
};

Calibration cal = {
    .xMin = 100,
    .xMax = 1900,
    .yMin = 100,
    .yMax = 1900
};

Xpt2046SoftSpi::Xpt2046SoftSpi(std::unique_ptr<Configuration> inConfiguration)
    : configuration(std::move(inConfiguration)) {
    assert(configuration != nullptr);
}

// Defensive check for NVS, put here just in case NVS is init after touch setup.
static void ensureNvsInitialized() {
    static bool initialized = false;
    if (initialized) return;

    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase(); // ignore error for safety
        result = nvs_flash_init();
    }

    initialized = (result == ESP_OK);
}

bool Xpt2046SoftSpi::startLvgl(lv_display_t* display) {

    // Create LVGL input device
    deviceHandle = lv_indev_create();
    if (!deviceHandle) {
        TT_LOG_E(TAG, "Failed to create LVGL input device");
        return false;
    }

    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(deviceHandle, touchReadCallback);
    lv_indev_set_user_data(deviceHandle, this);

    TT_LOG_I(TAG, "Xpt2046SoftSpi touch driver started successfully");
    return true;
}

bool Xpt2046SoftSpi::stop() {
    TT_LOG_I(TAG, "Stopping Xpt2046SoftSpi touch driver");

    // Stop LVLG if needed
    if (deviceHandle != nullptr) {
        stopLvgl();
    }

    return true;
}

int Xpt2046SoftSpi::readSPI(uint8_t command) {
    int result = 0;

    // Pull CS low for this transaction
    gpio_set_level(configuration->csPin, 0);
    ets_delay_us(1);

    // Send 8-bit command
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(configuration->mosiPin, command & (1 << i));
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    for (int i = 11; i >= 0; i--) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        if (gpio_get_level(configuration->misoPin)) {
            result |= (1 << i);
        }
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Pull CS high for this transaction
    gpio_set_level(configuration->csPin, 1);

    return result;
}

void Xpt2046SoftSpi::calibrate() {
    const int samples = 8; // More samples for better accuracy

    TT_LOG_I(TAG, "Calibration starting...");

    TT_LOG_I(TAG, "Touch TOP-LEFT corner");

    while (!isTouched()) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    int sumX = 0, sumY = 0;
    for (int i = 0; i < samples; i++) {
        sumX += readSPI(CMD_READ_X);
        sumY += readSPI(CMD_READ_Y);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    cal.xMin = sumX / samples;
    cal.yMin = sumY / samples;

    TT_LOG_I(TAG, "Top-left calibrated: xMin=%d, yMin=%d", cal.xMin, cal.yMin);

    TT_LOG_I(TAG, "Touch BOTTOM-RIGHT corner");

    while (!isTouched()) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    sumX = sumY = 0;
    for (int i = 0; i < samples; i++) {
        sumX += readSPI(CMD_READ_X);
        sumY += readSPI(CMD_READ_Y);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    cal.xMax = sumX / samples;
    cal.yMax = sumY / samples;

    TT_LOG_I(TAG, "Bottom-right calibrated: xMax=%d, yMax=%d", cal.xMax, cal.yMax);

    TT_LOG_I(TAG, "Calibration completed! xMin=%d, yMin=%d, xMax=%d, yMax=%d", cal.xMin, cal.yMin, cal.xMax, cal.yMax);
}

bool Xpt2046SoftSpi::loadCalibration() {
    TT_LOG_W(TAG, "Calibration load disabled (using fresh calibration only).");
    return false;
}

void Xpt2046SoftSpi::saveCalibration() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("xpt2046", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS for writing (%s)", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(handle, "cal", &cal, sizeof(cal));
    if (err == ESP_OK) {
        nvs_commit(handle);
        TT_LOG_I(TAG, "Calibration saved to NVS");
    } else {
        TT_LOG_E(TAG, "Failed to write calibration data to NVS (%s)", esp_err_to_name(err));
    }

    nvs_close(handle);
}

void Xpt2046SoftSpi::setCalibration(int xMin, int yMin, int xMax, int yMax) {
    cal.xMin = xMin;
    cal.yMin = yMin;
    cal.xMax = xMax;
    cal.yMax = yMax;
    TT_LOG_I(TAG, "Manual calibration set: xMin=%d, yMin=%d, xMax=%d, yMax=%d", xMin, yMin, xMax, yMax);
}

Point Xpt2046SoftSpi::getTouch() {

    const int samples = 8; // More samples for better accuracy
    int totalX = 0, totalY = 0;
    int validSamples = 0;

    for (int i = 0; i < samples; i++) {
        int rawX = readSPI(CMD_READ_X);
        int rawY = readSPI(CMD_READ_Y);

        // Only use valid readings
        if (rawX > 100 && rawX < 3900 && rawY > 100 && rawY < 3900) {
            totalX += rawX;
            totalY += rawY;
            validSamples++;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (validSamples == 0) {
        return Point {0, 0};
    }

    int rawX = totalX / validSamples;
    int rawY = totalY / validSamples;

    const int xRange = cal.xMax - cal.xMin;
    const int yRange = cal.yMax - cal.yMin;

    if (xRange <= 0 || yRange <= 0) {
        TT_LOG_W(TAG, "Invalid calibration: xRange=%d, yRange=%d", xRange, yRange);
        return Point {0, 0};
    }

    int x = (rawX - cal.xMin) * configuration->xMax / xRange;
    int y = (rawY - cal.yMin) * configuration->yMax / yRange;

    if (configuration->swapXy) std::swap(x, y);
    if (configuration->mirrorX) x = configuration->xMax - x;
    if (configuration->mirrorY) y = configuration->yMax - y;

    x = std::clamp(x, 0, (int)configuration->xMax);
    y = std::clamp(y, 0, (int)configuration->yMax);

    return Point {x, y};
}

bool Xpt2046SoftSpi::isTouched() {
    const int samples = 3;
    int xTotal = 0, yTotal = 0;
    int validSamples = 0;

    for (int i = 0; i < samples; i++) {
        int x = readSPI(CMD_READ_X);
        int y = readSPI(CMD_READ_Y);

        // Basic validity check - XPT2046 typically returns values in range 100-3900 when touched
        if (x > 100 && x < 3900 && y > 100 && y < 3900) {
            xTotal += x;
            yTotal += y;
            validSamples++;
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay between samples
    }
    gpio_set_level(configuration->csPin, 1);

    // Consider touched if we got valid readings
    bool touched = validSamples >= 2;

    // Debug logging (remove this once working)
    if (touched) {
        TT_LOG_I(TAG, "Touch detected: validSamples=%d, avgX=%d, avgY=%d", validSamples, xTotal / validSamples, yTotal / validSamples);
    }

    return touched;
}

void Xpt2046SoftSpi::touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    Xpt2046SoftSpi* touch = static_cast<Xpt2046SoftSpi*>(lv_indev_get_user_data(indev));

    if (touch && touch->isTouched()) {
        Point point = touch->getTouch();
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool Xpt2046SoftSpi::start() {
    ensureNvsInitialized();;

    TT_LOG_I(TAG, "Starting Xpt2046SoftSpi touch driver");

    // Configure GPIO pins
    gpio_config_t io_conf = {};

    // Configure MOSI, CLK, CS as outputs
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << configuration->mosiPin) |
        (1ULL << configuration->clkPin) |
        (1ULL << configuration->csPin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    if (gpio_config(&io_conf) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure output pins");
        return false;
    }

    // Configure MISO as input
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << configuration->misoPin);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    if (gpio_config(&io_conf) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure input pin");
        return false;
    }

    // Initialize pin states
    gpio_set_level(configuration->csPin, 1); // CS high
    gpio_set_level(configuration->clkPin, 0); // CLK low
    gpio_set_level(configuration->mosiPin, 0); // MOSI low

    TT_LOG_I(TAG, "GPIO configured: MOSI=%d, MISO=%d, CLK=%d, CS=%d", configuration->mosiPin, configuration->misoPin, configuration->clkPin, configuration->csPin);

    // Load or perform calibration
    bool calibrationValid = true; //loadCalibration() && !RERUN_CALIBRATE;
        if (calibrationValid) {
        // Check if calibration values are valid (xMin != xMax, yMin != yMax)
        if (cal.xMin == cal.xMax || cal.yMin == cal.yMax) {
            TT_LOG_W(TAG, "Invalid calibration detected: xMin=%d, xMax=%d, yMin=%d, yMax=%d", cal.xMin, cal.xMax, cal.yMin, cal.yMax);
            calibrationValid = false;
        }
    }

    if (!calibrationValid) {
        TT_LOG_W(TAG, "Calibration data not found, invalid, or forced recalibration");
        calibrate();
        saveCalibration();
    } else {
        TT_LOG_I(TAG, "Loaded calibration: xMin=%d, yMin=%d, xMax=%d, yMax=%d", cal.xMin, cal.yMin, cal.xMax, cal.yMax);
    }

    return true;
}

// Whether this device supports LVGL
bool Xpt2046SoftSpi::supportsLvgl() const {
    return true;
}

// Stop LVGL
bool Xpt2046SoftSpi::stopLvgl() {
    if (deviceHandle != nullptr) {
        lv_indev_delete(deviceHandle);
        deviceHandle = nullptr;
    }
    return true;
}

// Return driver instance if any
std::shared_ptr<tt::hal::touch::TouchDriver> Xpt2046SoftSpi::getTouchDriver() {
    return nullptr; // replace with actual driver later
}
