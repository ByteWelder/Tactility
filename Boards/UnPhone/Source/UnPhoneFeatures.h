#pragma once

#include "Thread.h"
#include "bq24295/Bq24295.h"
#include <esp_io_expander_tca95xx_16bit.h>

/**
 * Easy access to GPIO pins
 */
class UnPhoneFeatures {

private:

    esp_io_expander_handle_t ioExpander = nullptr;
    Bq24295 batteryManagement = Bq24295(I2C_NUM_0);
    tt::Thread buttonHandlingThread;
    bool buttonHandlingThreadInterruptRequest = false;

    bool initNavButtons();
    static bool initOutputPins();
    static bool initPowerSwitch();
    bool initGpioExpander();

public:

    UnPhoneFeatures() = default;
    ~UnPhoneFeatures();

    bool init();

    bool setBacklightPower(bool on) const;
    bool getBacklightPower(bool& on) const;
    bool setIrPower(bool on) const;
    bool setVibePower(bool on) const;
    bool setExpanderPower(bool on) const;

    bool isPowerSwitchOn() const;

    void turnPeripheralsOff() const;

    /** Battery management (BQ24295) will stop supplying power until USB connects */
    bool setShipping(bool on) const;

    void wakeOnPowerSwitch() const;

    bool isUsbPowerConnected() const;

    bool setRgbLed(bool red, bool green, bool blue) const;

    void printInfo() const;
};
