#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include <Tactility/TactilityCore.h>
#include <driver/i2c.h>

#include "ft6x36/FT6X36.h"

class Ft6x36Touch final : public tt::hal::touch::TouchDevice {

public:

    class Configuration {
    public:

        Configuration(
            i2c_port_t port,
            gpio_num_t pinInterrupt
        ) : port(port),
            pinInterrupt(pinInterrupt)
        {}

        i2c_port_t port;
        gpio_num_t pinInterrupt;
    };

private:

    std::unique_ptr<Configuration> configuration;
    lv_indev_t* _Nullable deviceHandle = nullptr;
    FT6X36 driver = FT6X36(configuration->port, configuration->pinInterrupt);
    tt::Thread driverThread;
    bool interruptDriverThread = false;
    tt::Mutex mutex;

    lv_point_t lastPoint = { .x = 0, .y = 0 };
    lv_indev_state_t lastState = LV_INDEV_STATE_RELEASED;

    bool shouldInterruptDriverThread();

public:

    explicit Ft6x36Touch(std::unique_ptr<Configuration> inConfiguration);
    ~Ft6x36Touch() final;

    std::string getName() const final { return "FT6x36"; }
    std::string getDescription() const final { return "I2C touch driver"; }

    bool start(lv_display_t* display) override;
    bool stop() override;

    void readLast(lv_indev_data_t* data);
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
    void driverThreadMain();
};
