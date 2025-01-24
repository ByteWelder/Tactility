#pragma once

#include "hal/i2c/I2cDevice.h"

#define BQ24295_ADDRESS 0x6BU

class Bq24295 : I2cDevice {

private:

    bool readChargeTermination(uint8_t& out) const;

public:

    enum class WatchDogTimer {
        Disabled = 0b000000,
        Enabled40s = 0b010000,
        Enabled80s = 0b100000,
        Enabled160s = 0b110000
    };

    explicit Bq24295(i2c_port_t port) : I2cDevice(port, BQ24295_ADDRESS) {}

    bool getWatchDogTimer(WatchDogTimer& out) const;
    bool setWatchDogTimer(WatchDogTimer in) const;

    bool isUsbPowerConnected() const;

    bool setBatFetOn(bool on) const;

    bool getStatus(uint8_t& value) const;
    bool getVersion(uint8_t& value) const;

    void printInfo() const;
};
