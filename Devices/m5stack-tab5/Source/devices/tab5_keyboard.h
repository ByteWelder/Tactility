#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <tactility/device.h>
#include <tactility/drivers/gpio.h>

struct Tab5KeyboardConfig {
    // Fixed 0x6D on this keyboard accessory - kept as a field for parity/documentation with
    // other dynamically-constructed devices in this project, not used to probe the bus.
    uint8_t address;
    // Native SoC GPIO (e.g. gpio0 pin 50) wired directly to the keyboard's INT line - not routed
    // through an IO expander, since it needs a real hardware ISR for responsive key events.
    // GPIO_PIN_SPEC_NONE falls back to polling REG_INT_STAT instead.
    struct GpioPinSpec pin_interrupt;
};

extern struct Driver tab5_keyboard_driver;

// Constructs and starts the keyboard accessory device on i2c2. Called from display_detect.cpp's
// on_display_detect_event() once i2c2 is up.
void tab5_create_keyboard(struct Device* i2c2);

// Returns true if the keyboard accessory currently ACKs on the I2C bus. Cheap bus probe, no
// debouncing - callers wanting hot-plug-stable state (e.g. tab5_keyboard_attach_detect.cpp)
// should debounce across their own polling interval.
bool tab5_keyboard_is_attached(struct Device* device);

// (Re)applies the device's register configuration - RGB mode, brightness, interrupt config, LED
// state. Volatile on this chip: reset to power-on defaults whenever the keyboard is unplugged and
// reconnected, so callers must call this again after confirming a reattach (see
// tab5_keyboard_attach_detect.cpp).
void tab5_keyboard_reinit(struct Device* device);

#ifdef __cplusplus
}
#endif
