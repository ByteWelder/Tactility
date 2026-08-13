#include "doctest.h"

#include <atomic>

#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/module.h>

namespace {

Module module = {
    .name = "device_get_put_test_module",
    .start = nullptr,
    .stop = nullptr
};

int start(Device*) { return ERROR_NONE; }
int stop(Device*) { return ERROR_NONE; }

Driver test_driver = {
    .name = "device_get_put_test_driver",
    .compatible = (const char*[]) { "device_get_put_test", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = nullptr,
    .device_type = nullptr,
    .owner = &module,
    .internal = nullptr,
};

} // namespace

TEST_CASE("device_get should succeed even when the device is not started") {
    Device device = { .name = "get_not_started", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);

    // Ref-counting brackets construct/destruct, not start/stop.
    CHECK_EQ(device_get(&device), ERROR_NONE);
    device_put(&device);

    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}

TEST_CASE("device_get should fail with ERROR_INVALID_STATE once the device has been destructed") {
    Device device = { .name = "get_after_destruct", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);

    CHECK_EQ(device_get(&device), ERROR_INVALID_STATE);

    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}

TEST_CASE("device_get should succeed once started, and device_put should release it") {
    Device device = { .name = "get_started", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);
    CHECK_EQ(device_start(&device), ERROR_NONE);

    CHECK_EQ(device_get(&device), ERROR_NONE);
    device_put(&device);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}

TEST_CASE("device_stop should succeed while a reference is held, but device_destruct should fail with ERROR_RESOURCE_BUSY until it is released") {
    static Device device = { .name = "get_put_concurrent", .config = nullptr, .parent = nullptr };
    static std::atomic<bool> acquired { false };
    static std::atomic<bool> release { false };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);
    CHECK_EQ(device_start(&device), ERROR_NONE);

    acquired = false;
    release = false;

    auto* thread = thread_alloc_full(
        "device_get_put_worker",
        4096,
        [](void*) -> int32_t {
            if (device_get(&device) != ERROR_NONE) {
                return 1;
            }
            acquired = true;
            while (!release.load()) {
                delay_millis(1);
            }
            device_put(&device);
            return 0;
        },
        nullptr,
        -1
    );

    CHECK_EQ(thread_start(thread), ERROR_NONE);

    while (!acquired.load()) {
        delay_millis(1);
    }

    // Held by the worker thread right now - device_stop() is independent of ref-counting, so it
    // still succeeds; only device_destruct() gates on outstanding refs.
    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_RESOURCE_BUSY);

    release = true;
    CHECK_EQ(thread_join(thread, 200, 1), ERROR_NONE);
    thread_free(thread);

    // Reference released - device_destruct() now succeeds.
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}

TEST_CASE("device_get_by_name should find and reference an added device regardless of started state, or fail if not found") {
    Device device = { .name = "get_by_name_device", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);

    Device* out = nullptr;
    CHECK_EQ(device_get_by_name("does_not_exist", &out), ERROR_NOT_FOUND);

    // Not started yet - lookup still succeeds, since it only requires the device to be added.
    CHECK_EQ(device_get_by_name("get_by_name_device", &out), ERROR_NONE);
    CHECK_EQ(out, &device);
    device_put(out);

    CHECK_EQ(device_start(&device), ERROR_NONE);
    CHECK_EQ(device_get_by_name("get_by_name_device", &out), ERROR_NONE);
    CHECK_EQ(out, &device);
    device_put(out);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}
