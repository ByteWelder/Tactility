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
#define CMD_READ_Y  0x90 // Try different commands if these don't work
#define CMD_READ_X  0xD0 // Alternative: 0x98 for Y, 0xD8 for X

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

    TT_LOG_I(TAG, "GPIO configured: MOSI=%d, MISO=%d, CLK=%d, CS=%d", 
             configuration->mosiPin, configuration->misoPin, 
             configuration->clkPin, configuration->csPin);

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
    
    // Pull CS low for this transaction
    gpio_set_level(configuration->csPin, 0);
    ets_delay_us(1);

    // Send 8-bit command
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(configuration->mosiPin, (command >> i) & 1);
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Read 16 bits (12-bit data + 4 bits padding)
    for (int i = 15; i >= 0; i--) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(1);
        if (gpio_get_level(configuration->misoPin)) {
            result |= (1 << i);
        }
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(1);
    }

    // Pull CS high to end transaction
    gpio_set_level(configuration->csPin, 1);
    ets_delay_us(1);

    // Return 12-bit data (shift right by 3, not 4, as data is typically in bits 14:3)
    return (result >> 3) & 0x0FFF;
}

void XPT2046_Bitbang::calibrate() {
    TT_LOG_I(TAG, "Calibration starting...");

    // Test raw readings first
    TT_LOG_I(TAG, "Testing raw SPI communication...");
    for (int i = 0; i < 5; i++) {
        int x = readSPI(CMD_READ_X);
        int y = readSPI(CMD_READ_Y);
        TT_LOG_I(TAG, "Raw test %d: X=%d, Y=%d", i, x, y);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    TT_LOG_I(TAG, "Touch the top-left corner and hold...");

    // Wait for touch with timeout
    int timeout = 30000; // 30 seconds
    int elapsed = 0;
    while (!isTouched() && elapsed < timeout) {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
        
        if (elapsed % 5000 == 0) {
            TT_LOG_I(TAG, "Still waiting for touch... (%d/%d seconds)", elapsed/1000, timeout/1000);
            int x = readSPI(CMD_READ_X);
            int y = readSPI(CMD_READ_Y);
            TT_LOG_I(TAG, "Current raw readings: X=%d, Y=%d", x, y);
        }
    }

    if (elapsed >= timeout) {
        TT_LOG_E(TAG, "Calibration timeout! No touch detected.");
        cal.xMin = 300; cal.yMin = 300;
        cal.xMax = 3700; cal.yMax = 3700;
        return;
    }

    TT_LOG_I(TAG, "Touch detected! Sampling top-left corner...");
    vTaskDelay(pdMS_TO_TICKS(500));  // wait for stable touch

    int xSum = 0, ySum = 0, samples = 8;
    for (int i = 0; i < samples; i++) {
        int x = readSPI(CMD_READ_X);
        int y = readSPI(CMD_READ_Y);
        xSum += x;
        ySum += y;
        TT_LOG_I(TAG, "Sample %d: X=%d, Y=%d", i, x, y);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    cal.xMin = xSum / samples;
    cal.yMin = ySum / samples;
    TT_LOG_I(TAG, "Top-left calibrated: xMin=%d, yMin=%d", cal.xMin, cal.yMin);

    // Wait for release
    TT_LOG_I(TAG, "Release touch and then touch bottom-right corner...");
    while (isTouched()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // ensure full release

    // Wait for bottom-right touch
    elapsed = 0;
    while (!isTouched() && elapsed < timeout) {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
        if (elapsed % 5000 == 0) {
            TT_LOG_I(TAG, "Waiting for bottom-right corner touch... (%d/%d seconds)", elapsed/1000, timeout/1000);
        }
    }

    if (elapsed >= timeout) {
        TT_LOG_E(TAG, "Calibration timeout on second touch!");
        return;
    }

    TT_LOG_I(TAG, "Bottom-right touch detected! Waiting for stable press...");
    vTaskDelay(pdMS_TO_TICKS(100));  // new short settle delay
    vTaskDelay(pdMS_TO_TICKS(500));  // same stable press delay as top-left

    xSum = 0; ySum = 0;
    for (int i = 0; i < samples; i++) {
        int x = readSPI(CMD_READ_X);
        int y = readSPI(CMD_READ_Y);
        xSum += x;
        ySum += y;
        TT_LOG_I(TAG, "Sample %d: X=%d, Y=%d", i, x, y);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    cal.xMax = xSum / samples;
    cal.yMax = ySum / samples;

    TT_LOG_I(TAG, "Calibration completed! xMin=%d, yMin=%d, xMax=%d, yMax=%d",
             cal.xMin, cal.yMin, cal.xMax, cal.yMax);
}

// Add back when correct calibration
// bool XPT2046_Bitbang::loadCalibration() {
//     nvs_handle_t handle;
//     esp_err_t err = nvs_open("xpt2046", NVS_READONLY, &handle);
//     if (err != ESP_OK) {
//         TT_LOG_W(TAG, "Calibration NVS namespace not found");
//         return false;
//     }

//     size_t size = sizeof(cal);
//     err = nvs_get_blob(handle, "cal", &cal, &size);
//     nvs_close(handle);

//     if (err != ESP_OK || size != sizeof(cal)) {
//         TT_LOG_W(TAG, "Failed to read calibration from NVS (%s)", esp_err_to_name(err));
//         return false;
//     }

//     return true;
// }

bool XPT2046_Bitbang::loadCalibration() {
    TT_LOG_W(TAG, "Calibration load disabled (using fresh calibration only).");
    return false;
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
        return Point{0, 0};
    }

    int rawX = totalX / validSamples;
    int rawY = totalY / validSamples;

    const int xRange = cal.xMax - cal.xMin;
    const int yRange = cal.yMax - cal.yMin;

    if (xRange <= 0 || yRange <= 0) {
        TT_LOG_W(TAG, "Invalid calibration: xRange=%d, yRange=%d", xRange, yRange);
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

    // Consider touched if we got valid readings
    bool touched = validSamples >= 2;
    
    // Debug logging (remove this once working)
    if (touched) {
        TT_LOG_I(TAG, "Touch detected: validSamples=%d, avgX=%d, avgY=%d", 
                 validSamples, xTotal/validSamples, yTotal/validSamples);
    }
    
    return touched;
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


// Zero-argument start
bool XPT2046_Bitbang::start() {
    // Default to LVGL-less startup if needed
    return startLvgl(nullptr);
}

// Whether this device supports LVGL
bool XPT2046_Bitbang::supportsLvgl() const {
    return true;
}

// Start with LVGL display
bool XPT2046_Bitbang::startLvgl(lv_display_t* display) {
    return start(display);
}

// Stop LVGL
bool XPT2046_Bitbang::stopLvgl() {
    cleanup();
    return true;
}

// Supports a separate touch driver? Yes/No
bool XPT2046_Bitbang::supportsTouchDriver() {
    return true;
    }

// Return driver instance if any
std::shared_ptr<tt::hal::touch::TouchDriver> XPT2046_Bitbang::getTouchDriver() {
    return nullptr; // replace with actual driver later
}
