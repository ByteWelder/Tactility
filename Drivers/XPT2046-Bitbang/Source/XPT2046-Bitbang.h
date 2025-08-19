#pragma once
#include "Tactility/hal/touch/TouchDevice.h"
#include <Tactility/TactilityCore.h>
#include "lvgl.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include <string>

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif

struct Point {
    int x;
    int y;
};

class XPT2046_Bitbang : public tt::hal::touch::TouchDevice {
public:
    class Configuration {
    public:
        Configuration(
            gpio_num_t mosiPin,
            gpio_num_t misoPin,
            gpio_num_t clkPin,
            gpio_num_t csPin,
            gpio_num_t irqPin,
            uint16_t xMax = TFT_WIDTH,
            uint16_t yMax = TFT_HEIGHT,
            bool swapXy = false,
            bool mirrorX = false,
            bool mirrorY = false
        );
        
        gpio_num_t mosiPin;
        gpio_num_t misoPin;
        gpio_num_t clkPin;
        gpio_num_t csPin;
        gpio_num_t irqPin;
        uint16_t xMax;
        uint16_t yMax;
        bool swapXy;
        bool mirrorX;
        bool mirrorY;
    };

private:
    static XPT2046_Bitbang* instance;
    std::unique_ptr<Configuration> configuration;
    lv_indev_t* deviceHandle = nullptr;
    
    int readSPI(uint8_t cmd);
    void cleanup();
    static void touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data);

public:
    explicit XPT2046_Bitbang(std::unique_ptr<Configuration> cfg);
    
    // Override Device pure virtual methods
    std::string getName() const override { return "XPT2046_Bitbang"; }
    std::string getDescription() const override { return "XPT2046 Touch Controller (Bitbang SPI)"; }
    
    // Override TouchDevice pure virtual methods
    bool start() override;
    bool stop() override;
    bool supportsLvgl() const override { return true; }
    bool startLvgl(lv_display_t* display) override;
    bool stopLvgl() override;
    lv_indev_t* getLvglIndev() override;
    bool supportsTouchDriver() override { return false; }
    std::shared_ptr<tt::hal::touch::TouchDriver> getTouchDriver() override { return nullptr; }
    
    // XPT2046 specific methods
    bool isTouched();
    Point getTouch();
    void calibrate();
    void setCalibration(int xMin, int yMin, int xMax, int yMax);
    
    static XPT2046_Bitbang* getInstance() { return instance; }
};