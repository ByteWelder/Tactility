#include "XPT2046-Bitbang.h"

#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <inttypes.h>

#define TAG "xpt2046_bitbang"

#define RERUN_CALIBRATE false
#define CMD_READ_Y  0x98 // Single-ended mode Y position command
#define CMD_READ_X  0xD8 // Single-ended mode X position command

XPT2046_Bitbang* XPT2046_Bitbang::instance = nullptr;

XPT2046_Bitbang::XPT2046_Bitbang(std::unique_ptr<Configuration> inConfiguration)
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

bool XPT2046_Bitbang::start(lv_display_t* display) {
    ensureNvsInitialized();
    
    TT_LOG_I(TAG, "Starting XPT2046 Bitbang touch driver");

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
    gpio_set_level(configuration->csPin, 1);    // CS high
    gpio_set_level(configuration->clkPin, 0);   // CLK low
    gpio_set_level(configuration->mosiPin, 0);  // MOSI low

    // Load or perform calibration
    bool calibrationValid = loadCalibration() && !RERUN_CALIBRATE;
    if (calibrationValid) {
        // Check if calibration values are valid (xMin != xMax, yMin != yMax)
        if (cal.xMin == cal.xMax || cal.yMin == cal.yMax) {
            TT_LOG_W(TAG, "Invalid calibration detected: xMin=%d, xMax=%d, yMin=%d, yMax=%d",
                     cal.xMin, cal.xMax, cal.yMin, cal.yMax);
            calibrationValid = false;
        }
    }

    if (!calibrationValid) {
        TT_LOG_W(TAG, "Calibration data not found, invalid, or forced recalibration");
        calibrate();
        saveCalibration();
    } else {
        TT_LOG_I(TAG, "Loaded calibration: xMin=%d, yMin=%d, xMax=%d, yMax=%d",
                 cal.xMin, cal.yMin, cal.xMax, cal.yMax);
    }

    // Create LVGL input device
    deviceHandle = lv_indev_create();
    if (!deviceHandle) {
        TT_LOG_E(TAG, "Failed to create LVGL input device");
        return false;
    }
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(deviceHandle, touchReadCallback);
    lv_indev_set_user_data(deviceHandle, this);

    instance = this;
    TT_LOG_I(TAG, "XPT2046 Bitbang touch driver started successfully");
    return true;
}

bool XPT2046_Bitbang::stop() {
    TT_LOG_I(TAG, "Stopping XPT2046 Bitbang touch driver");
    instance = nullptr;
    cleanup();
    return true;
}

void XPT2046_Bitbang::cleanup() {
    if (deviceHandle != nullptr) {
        lv_indev_delete(deviceHandle);
        deviceHandle = nullptr;
    }
}

int XPT2046_Bitbang::readSPI(uint8_t command) {
    int result = 0;

    // Send 8-bit command
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(configuration->mosiPin, (command >> i) & 1);
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Send 3 dummy clocks (acquisition time)
    for (int i = 0; i < 3; i++) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Read 16 bits (12-bit data left-aligned in bits 15:4)
    for (int i = 15; i >= 0; i--) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        if (gpio_get_level(configuration->misoPin)) {
            result |= (1 << i);
        }
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Return 12-bit data (discard bottom 4 bits)
    return result >> 4;
}

void XPT2046_Bitbang::calibrate() {
    TT_LOG_I(TAG, "Calibration starting...");

    // Wait until touch detected on top-left corner
    TT_LOG_I(TAG, "Touch the top-left corner and hold...");
    while (!isTouched()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Wait for stable touch

    gpio_set_level(configuration->csPin, 0);
    int xSum = 0, ySum = 0, samples = 8;
    for (int i = 0; i < samples; i++) {
        readSPI(CMD_READ_X);
        xSum += readSPI(0x00);
        readSPI(CMD_READ_Y);
        ySum += readSPI(0x00);
    }
    gpio_set_level(configuration->csPin, 1);
    cal.xMin = xSum / samples;
    cal.yMin = ySum / samples;

    // Wait until touch detected on bottom-right corner
    TT_LOG_I(TAG, "Touch the bottom-right corner and hold...");
    while (!isTouched()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Wait for stable touch

    gpio_set_level(configuration->csPin, 0);
    xSum = 0; ySum = 0;
    for (int i = 0; i < samples; i++) {
        readSPI(CMD_READ_X);
        xSum += readSPI(0x00);
        readSPI(CMD_READ_Y);
        ySum += readSPI(0x00);
    }
    gpio_set_level(configuration->csPin, 1);
    cal.xMax = xSum / samples;
    cal.yMax = ySum / samples;

    TT_LOG_I(TAG, "Calibration done! xMin=%d, yMin=%d, xMax=%d, yMax=%d",
             cal.xMin, cal.yMin, cal.xMax, cal.yMax);
}

bool XPT2046_Bitbang::loadCalibration() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("xpt2046", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        TT_LOG_W(TAG, "Calibration NVS namespace not found");
        return false;
    }

    size_t size = sizeof(cal);
    err = nvs_get_blob(handle, "cal", &cal, &size);
    nvs_close(handle);

    if (err != ESP_OK || size != sizeof(cal)) {
        TT_LOG_W(TAG, "Failed to read calibration from NVS (%s)", esp_err_to_name(err));
        return false;
    }

    return true;
}

void XPT2046_Bitbang::saveCalibration() {
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

void XPT2046_Bitbang::setCalibration(int xMin, int yMin, int xMax, int yMax) {
    cal.xMin = xMin;
    cal.yMin = yMin;
    cal.xMax = xMax;
    cal.yMax = yMax;
    TT_LOG_I(TAG, "Manual calibration set: xMin=%d, yMin=%d, xMax=%d, yMax=%d",
             xMin, yMin, xMax, yMax);
}

Point XPT2046_Bitbang::getTouch() {
    const int samples = 4;
    int totalX = 0, totalY = 0;

    gpio_set_level(configuration->csPin, 0);

    for (int i = 0; i < samples; i++) {
        // Send command and read X data once per sample
        readSPI(CMD_READ_X);      // send X command
        totalX += readSPI(0x00);  // read X data

        // Send command and read Y data once per sample
        readSPI(CMD_READ_Y);      // send Y command
        totalY += readSPI(0x00);  // read Y data
    }

    gpio_set_level(configuration->csPin, 1);

    int rawX = totalX / samples;
    int rawY = totalY / samples;

    const int xRange = cal.xMax - cal.xMin;
    const int yRange = cal.yMax - cal.yMin;

    if (xRange <= 0 || yRange <= 0) {
        TT_LOG_I(TAG, "Invalid calibration: xRange=%d, yRange=%d", xRange, yRange);
        return Point{0, 0};
    }

    int x = (rawX - cal.xMin) * configuration->xMax / xRange;
    int y = (rawY - cal.yMin) * configuration->yMax / yRange;

    if (configuration->swapXy) std::swap(x, y);
    if (configuration->mirrorX) x = configuration->xMax - x;
    if (configuration->mirrorY) y = configuration->yMax - y;

    x = std::clamp(x, 0, (int)configuration->xMax);
    y = std::clamp(y, 0, (int)configuration->yMax);

    return Point{x, y};
}

bool XPT2046_Bitbang::isTouched() {
    const int samples = 3;
    int total = 0;

    gpio_set_level(configuration->csPin, 0);
    for (int i = 0; i < samples; i++) {
        readSPI(CMD_READ_X);
        total += readSPI(0x00);
    }
    gpio_set_level(configuration->csPin, 1);

    int avg = total / samples;

    return (avg > 100 && avg < 4000);
}


void XPT2046_Bitbang::touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    XPT2046_Bitbang* touch = static_cast<XPT2046_Bitbang*>(lv_indev_get_user_data(indev));

    if (touch && touch->isTouched()) {
        Point point = touch->getTouch();
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
