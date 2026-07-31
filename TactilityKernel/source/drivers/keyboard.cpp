// SPDX-License-Identifier: Apache-2.0
#include <tactility/device.h>
#include <tactility/drivers/keyboard.h>
#include <tactility/error.h>

#define KEYBOARD_DRIVER_API(driver) ((struct KeyboardApi*)driver->api)

extern "C" {

error_t keyboard_read_key(Device* device, KeyboardKeyData* data) {
    const auto* driver = device_get_driver(device);
    return KEYBOARD_DRIVER_API(driver)->read_key(device, data);
}

error_t keyboard_get_backlight(Device* device, Device** backlight_device) {
    const auto* driver = device_get_driver(device);

    if (KEYBOARD_DRIVER_API(driver)->get_backlight == nullptr) {
        return ERROR_NOT_SUPPORTED;
    }

    return KEYBOARD_DRIVER_API(driver)->get_backlight(device, backlight_device);
}

void keyboard_notify_bound(Device* device) {
    const auto* driver = device_get_driver(device);

    if (KEYBOARD_DRIVER_API(driver)->notify_bound != nullptr) {
        KEYBOARD_DRIVER_API(driver)->notify_bound(device);
    }
}

const DeviceType KEYBOARD_TYPE {
    .name = "keyboard"
};

}
