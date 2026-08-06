// SPDX-License-Identifier: Apache-2.0
#include <tactility/device.h>
#include <tactility/drivers/trackball.h>
#include <tactility/error.h>

#define TRACKBALL_DRIVER_API(driver) ((struct TrackballApi*)driver->api)

extern "C" {

error_t trackball_read_delta(Device* device, int32_t* out_dx, int32_t* out_dy) {
    const auto* driver = device_get_driver(device);
    return TRACKBALL_DRIVER_API(driver)->read_delta(device, out_dx, out_dy);
}

error_t trackball_get_button_pressed(Device* device, bool* out_pressed) {
    const auto* driver = device_get_driver(device);
    return TRACKBALL_DRIVER_API(driver)->get_button_pressed(device, out_pressed);
}

const DeviceType TRACKBALL_TYPE {
    .name = "trackball"
};

}
