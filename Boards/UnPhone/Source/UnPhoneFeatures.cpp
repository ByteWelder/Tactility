#include "UnPhoneFeatures.h"
#include "Log.h"
#include "kernel/Kernel.h"
#include <driver/gpio.h>
#include <esp_sleep.h>

namespace pin {
    static const gpio_num_t BUTTON1 = GPIO_NUM_45; // left button
    static const gpio_num_t BUTTON2 = GPIO_NUM_0; // middle button
    static const gpio_num_t BUTTON3 = GPIO_NUM_21; // right button
    static const gpio_num_t IR_LEDS = GPIO_NUM_12;
    static const gpio_num_t LED_RED = GPIO_NUM_13;
    static const gpio_num_t POWER_SWITCH = GPIO_NUM_18;
} // namespace pin

namespace expanderpin {
    static const esp_io_expander_pin_num_t BACKLIGHT = IO_EXPANDER_PIN_NUM_2;
    static const esp_io_expander_pin_num_t EXPANDER_POWER = IO_EXPANDER_PIN_NUM_0; // enable exp brd if high
    static const esp_io_expander_pin_num_t LED_GREEN = IO_EXPANDER_PIN_NUM_9;
    static const esp_io_expander_pin_num_t LED_BLUE = IO_EXPANDER_PIN_NUM_13;
    static const esp_io_expander_pin_num_t USB_VSENSE = IO_EXPANDER_PIN_NUM_14;
    static const esp_io_expander_pin_num_t VIBE = IO_EXPANDER_PIN_NUM_7;
} // namespace expanderpin

#define TAG "unhpone_features"

bool UnPhoneFeatures::init() {
    TT_LOG_I(TAG, "init");

    uint64_t output_pin_mask =
        BIT64(pin::IR_LEDS) |
        BIT64(pin::LED_RED);

    gpio_config_t output_gpio_config = {
        .pin_bit_mask = output_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&output_gpio_config) != ESP_OK) {
        TT_LOG_E(TAG, "Output pin init failed");
        return false;
    }

    uint64_t input_pin_mask =
        BIT64(pin::BUTTON1) |
        BIT64(pin::BUTTON2) |
        BIT64(pin::BUTTON3);

    gpio_config_t input_gpio_config = {
        .pin_bit_mask = input_pin_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&input_gpio_config) != ESP_OK) {
        TT_LOG_E(TAG, "Input pin init failed");
        return false;
    }

    if (gpio_config(&input_gpio_config) != ESP_OK) {
        TT_LOG_E(TAG, "Input pin init failed");
        return false;
    }

    uint64_t power_pin_mask = BIT64(pin::POWER_SWITCH);

    gpio_config_t power_gpio_config = {
        .pin_bit_mask = power_pin_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_HIGH_LEVEL,
    };

    if (gpio_config(&power_gpio_config) != ESP_OK) {
        TT_LOG_E(TAG, "Power pin init failed");
        return false;
    }

    // ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_110 corresponds with 0x26 from the docs at
    // https://gitlab.com/hamishcunningham/unphonelibrary/-/blob/main/unPhone.h?ref_type=heads#L206
    if (esp_io_expander_new_i2c_tca95xx_16bit(I2C_NUM_0, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_110, &ioExpander) != ESP_OK) {
        TT_LOG_E(TAG, "IO expander init failed");
        return false;
    }
    assert(ioExpander != nullptr);

    // Output pins
    esp_io_expander_set_dir(ioExpander, expanderpin::BACKLIGHT, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(ioExpander, expanderpin::EXPANDER_POWER, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(ioExpander, expanderpin::LED_GREEN, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(ioExpander, expanderpin::LED_BLUE, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(ioExpander, expanderpin::VIBE, IO_EXPANDER_OUTPUT);
    // Input pins
    esp_io_expander_set_dir(ioExpander, expanderpin::USB_VSENSE, IO_EXPANDER_INPUT);

    return true;
}

void UnPhoneFeatures::printInfo() const {
    esp_io_expander_print_state(ioExpander);
    batteryManagement.printInfo();
    bool backlight_power;
    const char* backlight_power_state = getBacklightPower(backlight_power) && backlight_power ? "on" : "off";
    TT_LOG_I(TAG, "Backlight: %s", backlight_power_state);
}

bool UnPhoneFeatures::setRgbLed(bool red, bool green, bool blue) const {
    assert(ioExpander != nullptr);
    return gpio_set_level(pin::LED_RED, red ? 1U : 0U) == ESP_OK &&
        esp_io_expander_set_level(ioExpander, expanderpin::LED_GREEN, green ? 1U : 0U) == ESP_OK &&
        esp_io_expander_set_level(ioExpander, expanderpin::LED_BLUE, blue ? 1U : 0U) == ESP_OK;
}

bool UnPhoneFeatures::setBacklightPower(bool on) const {
    assert(ioExpander != nullptr);
    return esp_io_expander_set_level(ioExpander, expanderpin::BACKLIGHT, on ? 1U : 0U) == ESP_OK;
}

bool UnPhoneFeatures::getBacklightPower(bool& on) const {
    assert(ioExpander != nullptr);
    uint32_t level_mask;
    if (esp_io_expander_get_level(ioExpander, expanderpin::BACKLIGHT, &level_mask) == ESP_OK) {
        on = level_mask != 0U;
        return true;
    } else {
        return false;
    }
}

bool UnPhoneFeatures::setIrPower(bool on) const {
    assert(ioExpander != nullptr);
    return gpio_set_level(pin::IR_LEDS, on ? 1U : 0U) == ESP_OK;
}

bool UnPhoneFeatures::setVibePower(bool on) const {
    assert(ioExpander != nullptr);
    return esp_io_expander_set_level(ioExpander, expanderpin::VIBE, on ? 1U : 0U) == ESP_OK;
}

bool UnPhoneFeatures::setExpanderPower(bool on) const {
    assert(ioExpander != nullptr);
    return esp_io_expander_set_level(ioExpander, expanderpin::EXPANDER_POWER, on ? 1U : 0U) == ESP_OK;
}

bool UnPhoneFeatures::isPowerSwitchOn() const {
    return gpio_get_level(pin::POWER_SWITCH) > 0;
}

void UnPhoneFeatures::turnPeripheralsOff() const {
    setExpanderPower(false);
    setBacklightPower(false);
    setIrPower(false);
    setRgbLed(false, false, false);
    setVibePower(false);
}

bool UnPhoneFeatures::setShipping(bool on) const {
    if (on) {
        TT_LOG_W(TAG, "setShipping: on");
        uint8_t mask = (1 << 4) | (1 << 5);
        // REG05[5:4] = 00
        batteryManagement.setWatchDogBitOff(mask);
        // Set bit 5 to disable
        batteryManagement.setOperationControlBitOn(1 << 5);
    } else {
        TT_LOG_W(TAG, "setShipping: off");
        // REG05[5:4] = 01
        batteryManagement.setWatchDogBitOff(1 << 5);
        batteryManagement.setWatchDogBitOn(1 << 4);
        // Clear bit 5 to enable
        batteryManagement.setOperationControlBitOff(1 << 5);
    }
    return true;
}

void UnPhoneFeatures::wakeOnPowerSwitch() const {
    esp_sleep_enable_ext0_wakeup(pin::POWER_SWITCH, 1);
}

bool UnPhoneFeatures::isUsbPowerConnected() const {
    uint8_t status;
    if (batteryManagement.getStatus(status)) {
        return (status & 4U) != 0U;
    } else {
        return false;
    }
}
