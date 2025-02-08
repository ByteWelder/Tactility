#pragma once

#include <Bq24295.h>
#include <Tactility/Thread.h>
#include <esp_io_expander_tca95xx_16bit.h>

/**
 * Easy access to GPIO pins
 */
class UnPhoneFeatures final {

private:

    esp_io_expander_handle_t ioExpander = nullptr;
    tt::Thread buttonHandlingThread;
    bool buttonHandlingThreadInterruptRequest = false;

    bool initNavButtons();
    static bool initOutputPins();
    static bool initPowerSwitch();
    bool initGpioExpander();

    std::shared_ptr<Bq24295> batteryManagement;

public:

    explicit UnPhoneFeatures(std::shared_ptr<Bq24295> bq24295) : batteryManagement(std::move(bq24295)) {
        assert(batteryManagement != nullptr);
    }

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
