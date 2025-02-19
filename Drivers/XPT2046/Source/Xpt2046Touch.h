#pragma once

#include "Tactility/hal/touch/TouchDevice.h"

#include <Tactility/TactilityCore.h>

#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_touch.h>

class Xpt2046Touch : public tt::hal::touch::TouchDevice {

public:

    class Configuration {
    public:

        Configuration(
            esp_lcd_spi_bus_handle_t spiDevice,
            gpio_num_t spiPinCs,
            uint16_t xMax,
            uint16_t yMax,
            bool swapXy = false,
            bool mirrorX = false,
            bool mirrorY = false
        ) : spiDevice(spiDevice),
            spiPinCs(spiPinCs),
            xMax(xMax),
            yMax(yMax),
            swapXy(swapXy),
            mirrorX(mirrorX),
            mirrorY(mirrorY)
        {}

        esp_lcd_spi_bus_handle_t spiDevice;
        gpio_num_t spiPinCs;
        uint16_t xMax;
        uint16_t yMax;
        bool swapXy;
        bool mirrorX;
        bool mirrorY;
    };

private:

    static Xpt2046Touch* instance;

    std::unique_ptr<Configuration> configuration;
    esp_lcd_panel_io_handle_t _Nullable ioHandle = nullptr;
    esp_lcd_touch_handle_t _Nullable touchHandle = nullptr;
    lv_indev_t* _Nullable deviceHandle = nullptr;

    void cleanup();

public:

    explicit Xpt2046Touch(std::unique_ptr<Configuration> inConfiguration) : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    std::string getName() const final { return "XPT2046"; }
    std::string getDescription() const final { return "I2C touch driver"; }

    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }

    bool getVBat(float& outputVbat);

    /** Used for accessing getVBat() in Power driver */
    static Xpt2046Touch* getInstance() { return instance; }
};
