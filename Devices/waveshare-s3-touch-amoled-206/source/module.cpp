#include "bluetooth_reconnect.h"

#include <tactility/module.h>

extern "C" {

static error_t start() {
    waveshare_bluetooth_reconnect_start();
    return ERROR_NONE;
}

static error_t stop() {
    waveshare_bluetooth_reconnect_stop();
    return ERROR_NONE;
}

Module waveshare_s3_touch_amoled_206_module = {
    .name = "waveshare-s3-touch-amoled-206",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr
};

}
