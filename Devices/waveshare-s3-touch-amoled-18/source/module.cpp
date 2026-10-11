#include "bluetooth_reconnect.h"

#include <tactility/module.h>

// Defined in expander_reset.cpp (C++ linkage).
void waveshare_amoled18_expander_reset();

extern "C" {

static error_t start() {
    // The panel/touch reset lines hang off the TCA9554 expander, so release them before the kernel
    // starts the devicetree's display and touch devices.
    waveshare_amoled18_expander_reset();
    waveshare_bluetooth_reconnect_start();
    return ERROR_NONE;
}

static error_t stop() {
    waveshare_bluetooth_reconnect_stop();
    return ERROR_NONE;
}

Module waveshare_s3_touch_amoled_18_module = {
    .name = "waveshare-s3-touch-amoled-18",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr
};

}
