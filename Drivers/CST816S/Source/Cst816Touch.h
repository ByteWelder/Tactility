#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include <esp_lcd_touch.h>

class Cst816sTouch final : public tt::hal::touch::TouchDevice {

public:

    class Configuration {
    public:

        Configuration(
            i2c_port_t port,
            uint16_t xMax,
            uint16_t yMax,
            bool swapXy = false,
            bool mirrorX = false,
            bool mirrorY = false,
            gpio_num_t pinReset = GPIO_NUM_NC,
            gpio_num_t pinInterrupt = GPIO_NUM_NC,
            unsigned int pinResetLevel = 0,
            unsigned int pinInterruptLevel = 0
        ) : port(port),
            xMax(xMax),
            yMax(yMax),
            swapXY(swapXy),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            pinReset(pinReset),
            pinInterrupt(pinInterrupt),
            pinResetLevel(pinResetLevel),
            pinInterruptLevel(pinInterruptLevel)
        {}

        i2c_port_t port;
        uint16_t xMax;
        uint16_t yMax;
        bool swapXY;
        bool mirrorX;
        bool mirrorY;
        gpio_num_t pinReset;
        gpio_num_t pinInterrupt;
        unsigned int pinResetLevel;
        unsigned int pinInterruptLevel;
    };

private:

    std::unique_ptr<Configuration> configuration;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_touch_handle_t touchHandle = nullptr;
    lv_indev_t* _Nullable deviceHandle = nullptr;

    void cleanup();

public:

    explicit Cst816sTouch(std::unique_ptr<Configuration> inConfiguration) : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    std::string getName() const final { return "CST816S"; }
    std::string getDescription() const final { return "I2C touch driver"; }

    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
