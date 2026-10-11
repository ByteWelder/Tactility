// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/sleep_controller.h>
#include <tactility/device.h>
#include <tactility/system_event.h>

#define SLEEP_CONTROLLER_DRIVER_API(driver) ((SleepControllerApi*)driver->api)

extern "C" {

error_t sleep_controller_deep_sleep(Device* device) {
    system_event_emit(KERNEL_EVENT_DEEP_SLEEP_PREPARE, nullptr, 0);
    const auto* driver = device_get_driver(device);
    return SLEEP_CONTROLLER_DRIVER_API(driver)->deep_sleep(device);
}

const DeviceType SLEEP_CONTROLLER_TYPE {
    .name = "sleep_controller"
};

}
