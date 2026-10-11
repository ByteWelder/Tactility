#include "doctest.h"

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/sleep_controller.h>
#include <tactility/system_event.h>

static int call_order = 0;
static int prepare_event_order = 0;
static int deep_sleep_order = 0;

static void on_deep_sleep_prepare(SystemEvent* /*event*/, void* /*context*/) {
    prepare_event_order = ++call_order;
}

static error_t fake_deep_sleep(Device* /*device*/) {
    deep_sleep_order = ++call_order;
    return ERROR_NOT_SUPPORTED;
}

static const SleepControllerApi fake_sleep_controller_api = {
    .deep_sleep = fake_deep_sleep
};

static Driver fake_sleep_controller_driver = {
    .name = "fake_sleep_controller_driver",
    .compatible = (const char*[]) { "sleep_controller_test,fake", nullptr },
    .start_device = nullptr,
    .stop_device = nullptr,
    .api = &fake_sleep_controller_api,
    .device_type = &SLEEP_CONTROLLER_TYPE,
    .internal = nullptr,
};

TEST_CASE("sleep_controller_deep_sleep emits the prepare event before calling the driver") {
    call_order = 0;
    prepare_event_order = 0;
    deep_sleep_order = 0;

    static Device fake_device {
        .name = "fake_sleep_controller_device",
        .config = nullptr,
        .parent = nullptr,
    };

    CHECK_EQ(driver_construct_add(&fake_sleep_controller_driver), ERROR_NONE);
    CHECK_EQ(device_construct_add(&fake_device, "sleep_controller_test,fake"), ERROR_NONE);
    CHECK_EQ(device_start(&fake_device), ERROR_NONE);
    CHECK_EQ(system_event_callback_add(KERNEL_EVENT_DEEP_SLEEP_PREPARE, on_deep_sleep_prepare, nullptr), ERROR_NONE);

    CHECK_EQ(sleep_controller_deep_sleep(&fake_device), ERROR_NOT_SUPPORTED);
    CHECK_EQ(prepare_event_order, 1);
    CHECK_EQ(deep_sleep_order, 2);

    CHECK_EQ(system_event_callback_remove(KERNEL_EVENT_DEEP_SLEEP_PREPARE, on_deep_sleep_prepare), ERROR_NONE);
    CHECK_EQ(device_stop(&fake_device), ERROR_NONE);
    CHECK_EQ(device_remove(&fake_device), ERROR_NONE);
    CHECK_EQ(device_destruct(&fake_device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&fake_sleep_controller_driver), ERROR_NONE);
}
