#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"  // For esp_rom_delay_us

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode = 0>
class SoftSPI {
public:
    SoftSPI() : _delay(2), _cke(MODE_CPHA(Mode)), _ckp(MODE_CPOL(Mode)), _order(MSBFIRST) {}

    static constexpr uint8_t MSBFIRST = 0;  // Public constant
    static constexpr uint8_t LSBFIRST = 1;  // Public constant

    void begin() {
        fastPinMode(MisoPin, false);  // Input
        fastPinMode(MosiPin, true);   // Output
        fastPinMode(SckPin, true);    // Output
        fastDigitalWrite(SckPin, _ckp);  // Set initial clock polarity
        fastDigitalWrite(MosiPin, 0);    // Idle low
    }

    void end() {
        fastPinMode(MisoPin, false);
        fastPinMode(MosiPin, false);
        fastPinMode(SckPin, false);
    }

    void setBitOrder(uint8_t order) {
        _order = order & 1;  // 0 = MSBFIRST, 1 = LSBFIRST
    }

    void setDataMode(uint8_t mode) {
        switch (mode) {
            case 0: _ckp = 0; _cke = 0; break;  // Mode 0: CPOL=0, CPHA=0
            case 1: _ckp = 0; _cke = 1; break;  // Mode 1: CPOL=0, CPHA=1
            case 2: _ckp = 1; _cke = 0; break;  // Mode 2: CPOL=1, CPHA=0
            case 3: _ckp = 1; _cke = 1; break;  // Mode 3: CPOL=1, CPHA=1
            default: return;
        }
        fastDigitalWrite(SckPin, _ckp);  // Update clock polarity
    }

    void setClockDivider(uint32_t div) {
        // Map Arduino-style dividers to µs delays (assuming 240MHz ESP32 clock)
        switch (div) {
            case 2:   _delay = 1; break;   // ~500kHz
            case 4:   _delay = 2; break;   // ~250kHz
            case 8:   _delay = 4; break;   // ~125kHz
            case 16:  _delay = 8; break;   // ~62.5kHz
            case 32:  _delay = 16; break;  // ~31.25kHz
            case 64:  _delay = 32; break;  // ~15.625kHz
            case 128: _delay = 64; break;  // ~7.8125kHz
            default:  _delay = 2; break;   // Default to ~250kHz
        }
    }

    uint8_t transfer(uint8_t val) {
        uint8_t out = 0;
        if (_order == LSBFIRST) {
            // Reverse bits for LSBFIRST
            val = ((val & 0x01) << 7) | ((val & 0x02) << 5) | ((val & 0x04) << 3) | ((val & 0x08) << 1) |
                  ((val & 0x10) >> 1) | ((val & 0x20) >> 3) | ((val & 0x40) >> 5) | ((val & 0x80) >> 7);
        }

        uint8_t del = _delay >> 1;  // Half delay for each phase
        int sck = _ckp ? 1 : 0;     // Initial clock state

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (_cke) {  // CPHA=1: Shift clock first
                sck ^= 1;
                fastDigitalWrite(SckPin, sck);
                esp_rom_delay_us(del);
            }

            // Write bit
            fastDigitalWrite(MosiPin, (val & (1 << (_order == MSBFIRST ? 7 - bit : bit))) ? 1 : 0);
            esp_rom_delay_us(del);

            // Toggle clock
            sck ^= 1;
            fastDigitalWrite(SckPin, sck);

            // Read bit
            uint8_t bval = fastDigitalRead(MisoPin);
            if (_order == MSBFIRST) {
                out = (out << 1) | bval;
            } else {
                out = (out >> 1) | (bval << 7);
            }
            esp_rom_delay_us(del);

            if (!_cke) {  // CPHA=0: Shift clock after read
                sck ^= 1;
                fastDigitalWrite(SckPin, sck);
            }
        }
        return out;
    }

    uint16_t transfer16(uint16_t data) {
        union {
            uint16_t val;
            struct {
                uint8_t lsb;
                uint8_t msb;
            };
        } in, out;

        in.val = data;
        if (_order == MSBFIRST) {
            out.msb = transfer(in.msb);
            out.lsb = transfer(in.lsb);
        } else {
            out.lsb = transfer(in.lsb);
            out.msb = transfer(in.msb);
        }
        return out.val & 0x0FFF;  // Mask to 12 bits for XPT2046
    }

    void beginTransaction() {}
    void endTransaction() {}

private:
    static constexpr bool MODE_CPHA(uint8_t mode) { return (mode & 1) != 0; }
    static constexpr bool MODE_CPOL(uint8_t mode) { return (mode & 2) != 0; }

    uint8_t _delay;  // Delay in µs per half-cycle
    uint8_t _cke;    // Clock phase (CPHA)
    uint8_t _ckp;    // Clock polarity (CPOL)
    uint8_t _order;  // Bit order (MSBFIRST or LSBFIRST)

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
