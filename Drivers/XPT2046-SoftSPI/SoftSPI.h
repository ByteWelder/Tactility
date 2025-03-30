#pragma once

#include "driver/gpio.h"

template<uint8_t MisoPin, uint8_t MosiPin, uint8_t SckPin, uint8_t Mode = 0>
class SoftSPI {
public:
    void begin() {
        fastPinMode(MisoPin, false);
        fastPinMode(MosiPin, true);
        fastPinMode(SckPin, true);
        fastDigitalWrite(MosiPin, !MODE_CPHA(Mode));
        fastDigitalWrite(SckPin, MODE_CPOL(Mode));
    }

    uint8_t transfer(uint8_t data) {
        uint8_t rx = 0;
        for (int i = 7; i >= 0; i--) {
            fastDigitalWrite(MosiPin, (data >> i) & 1);
            fastDigitalWrite(SckPin, MODE_CPHA(Mode) ? MODE_CPOL(Mode) : !MODE_CPOL(Mode));
            if (fastDigitalRead(MisoPin)) rx |= (1 << i);
            fastDigitalWrite(SckPin, MODE_CPHA(Mode) ? !MODE_CPOL(Mode) : MODE_CPOL(Mode));
        }
        return rx;
    }

    uint16_t transfer16(uint8_t data) {
        uint16_t rx = transfer(data);
        rx = (rx << 8) | transfer(0);
        return rx;
    }

private:
    static constexpr bool MODE_CPHA(uint8_t mode) { return (mode & 1) != 0; }
    static constexpr bool MODE_CPOL(uint8_t mode) { return (mode & 2) != 0; }

    static inline void fastDigitalWrite(gpio_num_t pin, bool level) {
        gpio_set_level(pin, level);
    }

    static inline bool fastDigitalRead(gpio_num_t pin) {
        return gpio_get_level(pin) != 0;
    }

    static inline void fastPinMode(gpio_num_t pin, bool mode) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin),
            .mode = mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
};
