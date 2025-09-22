#include <Tactility/hal/gpio/Gpio.h>

#ifdef ESP_PLATFORM
#include <driver/gpio.h>
#endif

namespace tt::hal::gpio {

#ifdef ESP_PLATFORM

constexpr gpio_num_t toEspPin(Pin pin) { return static_cast<gpio_num_t>(pin); }

constexpr gpio_mode_t toEspGpioMode(Mode mode) {
    switch (mode) {
        case Mode::Input:
            return GPIO_MODE_INPUT;
        case Mode::Output:
            return GPIO_MODE_OUTPUT;
        case Mode::OutputOpenDrain:
            return GPIO_MODE_OUTPUT_OD;
        case Mode::InputOutput:
            return GPIO_MODE_INPUT_OUTPUT;
        case Mode::InputOutputOpenDrain:
            return GPIO_MODE_INPUT_OUTPUT_OD;
        case Mode::Disable:
        default:
            return GPIO_MODE_DISABLE;
    }
}

#endif

bool getLevel(Pin pin) {
#ifdef ESP_PLATFORM
    return gpio_get_level(toEspPin(pin)) == 1;
#else
    return false;
#endif
}

bool setLevel(Pin pin, bool level) {
#ifdef ESP_PLATFORM
    return gpio_set_level(toEspPin(pin), level) == ESP_OK;
#else
    return true;
#endif
}

int getPinCount() {
#ifdef ESP_PLATFORM
    return GPIO_NUM_MAX;
#else
    return 16;
#endif
}

bool configureWithPinBitmask(uint64_t pinBitMask, Mode mode, bool pullUp, bool pullDown) {
#ifdef ESP_PLATFORM
    gpio_config_t sd_gpio_config = {
        .pin_bit_mask = pinBitMask,
        .mode = toEspGpioMode(mode),
        .pull_up_en = pullUp ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = pullDown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&sd_gpio_config) == ESP_OK;
#else
    return true;
#endif
}

bool configure(Pin pin, Mode mode, bool pullUp, bool pullDown) {
#ifdef ESP_PLATFORM
    return configureWithPinBitmask(BIT64(toEspPin(pin)), mode, pullUp, pullDown);
#else
    return true;
#endif
}

bool setMode(Pin pin, Mode mode) {
#ifdef ESP_PLATFORM
    return gpio_set_direction(toEspPin(pin), toEspGpioMode(mode)) == ESP_OK;
#endif
    return true;
}

}
