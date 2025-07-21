#include "XPT2046-Bitbang.h"

#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <esp_vfs_fat.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>

#include <fstream>
#include <sstream>

#define TAG "xpt2046_bitbang"

#define RERUN_CALIBRATE false
#define CMD_READ_Y  0x90 // Command for XPT2046 to read Y position
#define CMD_READ_X  0xD0 // Command for XPT2046 to read X position
#define CALIBRATION_FILE "/spiffs/calxpt2046.txt"

XPT2046_Bitbang* XPT2046_Bitbang::instance = nullptr;

XPT2046_Bitbang::XPT2046_Bitbang(std::unique_ptr<Configuration> inConfiguration) 
    : configuration(std::move(inConfiguration)) {
    assert(configuration != nullptr);
}

bool XPT2046_Bitbang::start(lv_display_t* display) {
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
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable pull-up for MISO
    
    if (gpio_config(&io_conf) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure input pin");
        return false;
    }
    
    // Initialize pin states
    gpio_set_level(configuration->csPin, 1);    // CS high (inactive)
    gpio_set_level(configuration->clkPin, 0);   // CLK low
    gpio_set_level(configuration->mosiPin, 0);  // MOSI low
    
    // Initialize SPIFFS for calibration storage
    initializeSPIFFS();
    
    // Load or perform calibration
    if (!loadCalibration() || RERUN_CALIBRATE) {
        TT_LOG_W(TAG, "Calibration data not found or forced recalibration");
        calibrate();
        saveCalibration();
    } else {
        TT_LOG_I(TAG, "Loaded calibration: xMin=%d, yMin=%d, xMax=%d, yMax=%d", 
                 cal.xMin, cal.yMin, cal.xMax, cal.yMax);
    }
    
    // Create LVGL input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchReadCallback;
    indev_drv.user_data = this;
    
    deviceHandle = lv_indev_drv_register(&indev_drv);
    if (deviceHandle == nullptr) {
        TT_LOG_E(TAG, "Failed to register LVGL input device");
        return false;
    }
    
    // Associate with display
    lv_indev_set_display(deviceHandle, display);
    
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

void XPT2046_Bitbang::initializeSPIFFS() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
}

int XPT2046_Bitbang::readSPI(uint8_t command) {
    int result = 0;
    
    // Send command byte
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(configuration->mosiPin, (command >> i) & 1);
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(10);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(10);
    }
    
    // Read 12-bit result
    for (int i = 11; i >= 0; i--) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(10);
        if (gpio_get_level(configuration->misoPin)) {
            result |= (1 << i);
        }
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(10);
    }
    
    return result;
}

void XPT2046_Bitbang::calibrate() {
    TT_LOG_I(TAG, "Calibration starting...");
    TT_LOG_I(TAG, "Touch the top-left corner, hold it down for 3 seconds...");
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    gpio_set_level(configuration->csPin, 0);
    cal.xMin = readSPI(CMD_READ_X);
    cal.yMin = readSPI(CMD_READ_Y);
    gpio_set_level(configuration->csPin, 1);
    
    TT_LOG_I(TAG, "Touch the bottom-right corner, hold it down for 3 seconds...");
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    gpio_set_level(configuration->csPin, 0);
    cal.xMax = readSPI(CMD_READ_X);
    cal.yMax = readSPI(CMD_READ_Y);
    gpio_set_level(configuration->csPin, 1);
    
    TT_LOG_I(TAG, "Calibration done! xMin=%d, yMin=%d, xMax=%d, yMax=%d", 
             cal.xMin, cal.yMin, cal.xMax, cal.yMax);
}

bool XPT2046_Bitbang::loadCalibration() {
    std::ifstream file(CALIBRATION_FILE);
    if (!file.is_open()) {
        TT_LOG_W(TAG, "Calibration file not found");
        return false;
    }
    
    std::string line;
    if (std::getline(file, line)) cal.xMin = std::stoi(line);
    if (std::getline(file, line)) cal.yMin = std::stoi(line);
    if (std::getline(file, line)) cal.xMax = std::stoi(line);
    if (std::getline(file, line)) cal.yMax = std::stoi(line);
    
    file.close();
    return true;
}

void XPT2046_Bitbang::saveCalibration() {
    std::ofstream file(CALIBRATION_FILE);
    if (!file.is_open()) {
        TT_LOG_E(TAG, "Failed to open calibration file for writing");
        return;
    }
    
    file << cal.xMin << std::endl;
    file << cal.yMin << std::endl;
    file << cal.xMax << std::endl;
    file << cal.yMax << std::endl;
    
    file.close();
    TT_LOG_I(TAG, "Calibration saved");
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
    gpio_set_level(configuration->csPin, 0);
    
    int x = readSPI(CMD_READ_X);
    int y = readSPI(CMD_READ_Y);
    
    gpio_set_level(configuration->csPin, 1);
    
    // Map raw coordinates to screen coordinates
    x = (x - cal.xMin) * configuration->xMax / (cal.xMax - cal.xMin);
    y = (y - cal.yMin) * configuration->yMax / (cal.yMax - cal.yMin);
    
    // Apply transformations
    if (configuration->swapXy) {
        int temp = x;
        x = y;
        y = temp;
    }
    
    if (configuration->mirrorX) {
        x = configuration->xMax - x;
    }
    
    if (configuration->mirrorY) {
        y = configuration->yMax - y;
    }
    
    // Clamp to screen bounds
    if (x > configuration->xMax) x = configuration->xMax;
    if (x < 0) x = 0;
    if (y > configuration->yMax) y = configuration->yMax;
    if (y < 0) y = 0;
    
    return Point{x, y};
}

bool XPT2046_Bitbang::isTouched() {
    // Simple touch detection - read a coordinate and check if it's in valid range
    gpio_set_level(configuration->csPin, 0);
    int x = readSPI(CMD_READ_X);
    gpio_set_level(configuration->csPin, 1);
    
    // If the reading is significantly different from the no-touch state, consider it touched
    // XPT2046 typically reads around 4095 when not touched
    return (x < 4000 && x > 100);
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