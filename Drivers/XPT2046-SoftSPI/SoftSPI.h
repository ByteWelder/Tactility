#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"  // For esp_rom_delay_us

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode = 0>
class SoftSPI {
public:
    void begin() {
        fastPinMode(MisoPin, false);  // Input
        fastPinMode(MosiPin, true);   // Output
        fastPinMode(SckPin, true);    // Output
        fastDigitalWrite(MosiPin, !MODE_CPHA(Mode));
        fastDigitalWrite(SckPin, MODE_CPOL(Mode));
    }

    uint8_t transfer(uint8_t data) {
        uint8_t rx = 0;
        for (int i = 7; i >= 0; i--) {
            fastDigitalWrite(MosiPin, (data >> i) & 1);
            if (MODE_CPHA(Mode)) {
                fastDigitalWrite(SckPin, !MODE_CPOL(Mode));  // Clock low
                esp_rom_delay_us(1);  // ~500kHz, adjustable
                rx = rx << 1 | fastDigitalRead(MisoPin);
                fastDigitalWrite(SckPin, MODE_CPOL(Mode));   // Clock high
            } else {
                fastDigitalWrite(SckPin, !MODE_CPOL(Mode));  // Clock high
                esp_rom_delay_us(1);
                rx = rx << 1 | fastDigitalRead(MisoPin);
                fastDigitalWrite(SckPin, MODE_CPOL(Mode));   // Clock low
            }
            esp_rom_delay_us(1);
        }
        return rx;
    }

    uint16_t transfer16(uint8_t data) {
        uint16_t rx = transfer(data);
        rx = (rx << 8) | transfer(0x00);
        return rx & 0x0FFF;  // Mask to 12 bits (XPT2046 is 12-bit ADC)
    }

    void beginTransaction() {
        // No-op for SoftSPI
    }

    void endTransaction() {
        // No-op for SoftSPI
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
