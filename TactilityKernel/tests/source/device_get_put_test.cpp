#include "doctest.h"

#include <atomic>

#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/driver.h>

namespace {

int start(Device*) { return ERROR_NONE; }
int stop(Device*) { return ERROR_NONE; }

Driver test_driver = {
    .name = "device_get_put_test_driver",
    .compatible = (const char*[]) { "device_get_put_test", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = nullptr,
    .device_type = nullptr,
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

namespace {

const DeviceType typed_test_type = { .name = "device_get_put_test_type" };

Driver typed_test_driver = {
    .name = "device_get_put_test_typed_driver",
    .compatible = (const char*[]) { "device_get_put_test_typed", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = nullptr,
    .device_type = &typed_test_type,
    .internal = nullptr,
};

std::atomic<bool> blocking_start_entered { false };
std::atomic<bool> blocking_start_release { false };

int blocking_start(Device*) {
    blocking_start_entered = true;
    while (!blocking_start_release.load()) {
        delay_millis(1);
    }
    return ERROR_NONE;
}

Driver blocking_test_driver = {
    .name = "device_get_put_test_blocking_driver",
    .compatible = (const char*[]) { "device_get_put_test_blocking", nullptr },
    .start_device = blocking_start,
    .stop_device = stop,
    .api = nullptr,
    .device_type = nullptr,
    .internal = nullptr,
};

} // namespace

TEST_CASE("device_get_first_by_type should reference a device that is not started") {
    Device device = { .name = "get_first_by_type_device", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&typed_test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &typed_test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);

    Device* out = nullptr;
    CHECK_EQ(device_get_first_by_type(&typed_test_type, &out), ERROR_NONE);
    CHECK_EQ(out, &device);
    CHECK_EQ(device_start(out), ERROR_NONE);
    device_put(out);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&typed_test_driver), ERROR_NONE);
}

TEST_CASE("device_start should fail for a referenced device after device_remove") {
    Device device = { .name = "start_after_remove", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);

    CHECK_EQ(device_get(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_start(&device), ERROR_INVALID_STATE);
    CHECK_EQ(device_destruct(&device), ERROR_RESOURCE_BUSY);
    device_put(&device);

    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&test_driver), ERROR_NONE);
}

TEST_CASE("device_remove should fail while the device is being started") {
    static Device device = { .name = "remove_while_starting", .config = nullptr, .parent = nullptr };

    CHECK_EQ(driver_construct_add(&blocking_test_driver), ERROR_NONE);
    CHECK_EQ(device_construct(&device), ERROR_NONE);
    device_set_driver(&device, &blocking_test_driver);
    CHECK_EQ(device_add(&device), ERROR_NONE);

    blocking_start_entered = false;
    blocking_start_release = false;
    auto* thread = thread_alloc_full(
        "device_remove_starter",
        4096,
        [](void*) -> int32_t { return device_start(&device) == ERROR_NONE ? 0 : 1; },
        nullptr,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);
    while (!blocking_start_entered.load()) {
        delay_millis(1);
    }

    CHECK_EQ(device_remove(&device), ERROR_INVALID_STATE);

    blocking_start_release = true;
    CHECK_EQ(thread_join(thread, 200, 1), ERROR_NONE);
    CHECK_EQ(thread_get_return_code(thread), 0);
    thread_free(thread);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&blocking_test_driver), ERROR_NONE);
}
