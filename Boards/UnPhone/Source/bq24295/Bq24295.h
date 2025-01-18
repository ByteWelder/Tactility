#pragma once

#include "hal/i2c/I2cDevice.h"

#define BQ24295_ADDRESS 0x6BU

class Bq24295 : I2cDevice {

public:

    explicit Bq24295(i2c_port_t port) : I2cDevice(port, BQ24295_ADDRESS) {}

    bool getWatchDog(uint8_t value) const;
    bool setWatchDogBitOn(uint8_t mask) const;
    bool setWatchDogBitOff(uint8_t mask) const;

    bool getOperationControl(uint8_t value) const;
    bool setOperationControlBitOn(uint8_t mask) const;
    bool setOperationControlBitOff(uint8_t mask) const;

    bool getStatus(uint8_t& value) const;
    bool getVersion(uint8_t& value) const;

    void printInfo() const;
};