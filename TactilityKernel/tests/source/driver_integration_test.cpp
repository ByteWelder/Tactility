#include "doctest.h"
#include <tactility/driver.h>
#include <tactility/device.h>
#include <tactility/driver_internal.h>

struct IntegrationDriverConfig {
    int startResult;
    int stopResult;
};

static int startCalled = 0;
static int stopCalled = 0;

#define integration_data(device) static_cast<IntegrationDriverData*>(device_get_driver_data(device))
#define integration_config(device) static_cast<const IntegrationDriverConfig*>(device->config)

static int start(Device* device) {
    startCalled++;
    return integration_config(device)->startResult;
}

static int stop(Device* device) {
    stopCalled++;
    return integration_config(device)->stopResult;
}

static Driver integration_driver = {
    .name = "integration_test_driver",
    .compatible = (const char*[]) { "integration", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = nullptr,
    .device_type = nullptr,
    .internal = nullptr,
};

TEST_CASE("driver with with start success and stop success should start and stop a device") {
    startCalled = 0;
    stopCalled = 0;
    static const IntegrationDriverConfig config {
        .startResult = 0,
        .stopResult = 0
    };

    static Device integration_device {
        .name = "integration_device",
        .config = &config,
        .parent = nullptr,
    };

    CHECK_EQ(driver_construct_add(&integration_driver), ERROR_NONE);

    CHECK_EQ(device_construct(&integration_device), ERROR_NONE);
    device_add(&integration_device);
    CHECK_EQ(startCalled, 0);
    CHECK_EQ(driver_bind(&integration_driver, &integration_device), ERROR_NONE);
    CHECK_EQ(startCalled, 1);
    CHECK_EQ(stopCalled, 0);
    CHECK_EQ(driver_unbind(&integration_driver, &integration_device), ERROR_NONE);
    CHECK_EQ(stopCalled, 1);
    CHECK_EQ(device_remove(&integration_device), ERROR_NONE);
    CHECK_EQ(device_destruct(&integration_device), ERROR_NONE);

    CHECK_EQ(driver_remove_destruct(&integration_driver), ERROR_NONE);
}

TEST_CASE("driver_begin_destroy refuses new binds and is refused while a device is bound") {
    static const IntegrationDriverConfig config { .startResult = 0, .stopResult = 0 };
    static Device first_device { .name = "destroy_first_device", .config = &config, .parent = nullptr };
    static Device second_device { .name = "destroy_second_device", .config = &config, .parent = nullptr };
    static const char* destroy_compatible[] = { "destroy", nullptr };
    static Driver destroy_driver = {
        .name = "destroy_test_driver",
        .compatible = destroy_compatible,
        .start_device = start,
        .stop_device = stop,
    };

    REQUIRE_EQ(driver_construct_add(&destroy_driver), ERROR_NONE);
    REQUIRE_EQ(device_construct(&first_device), ERROR_NONE);
    REQUIRE_EQ(device_add(&first_device), ERROR_NONE);
    REQUIRE_EQ(device_construct(&second_device), ERROR_NONE);
    REQUIRE_EQ(device_add(&second_device), ERROR_NONE);

    // Bound: busy, and nothing changed so binding still works
    REQUIRE_EQ(driver_bind(&destroy_driver, &first_device), ERROR_NONE);
    CHECK_EQ(driver_begin_destroy(&destroy_driver), ERROR_RESOURCE_BUSY);
    CHECK_EQ(driver_bind(&destroy_driver, &second_device), ERROR_NONE);
    CHECK_EQ(driver_unbind(&destroy_driver, &second_device), ERROR_NONE);
    CHECK_EQ(driver_unbind(&destroy_driver, &first_device), ERROR_NONE);
    CHECK_FALSE(driver_is_in_use(&destroy_driver));

    // Free: from now on binds are refused, until the destroy is cancelled
    REQUIRE_EQ(driver_begin_destroy(&destroy_driver), ERROR_NONE);
    CHECK_EQ(driver_bind(&destroy_driver, &first_device), ERROR_INVALID_STATE);
    CHECK_FALSE(driver_is_in_use(&destroy_driver));
    driver_cancel_destroy(&destroy_driver);
    CHECK_EQ(driver_bind(&destroy_driver, &first_device), ERROR_NONE);
    CHECK_EQ(driver_unbind(&destroy_driver, &first_device), ERROR_NONE);

    // A failed start doesn't leave the driver counted as in use
    static const IntegrationDriverConfig failing { .startResult = ERROR_RESOURCE, .stopResult = 0 };
    static Device failing_device { .name = "destroy_failing_device", .config = &failing, .parent = nullptr };
    REQUIRE_EQ(device_construct(&failing_device), ERROR_NONE);
    REQUIRE_EQ(device_add(&failing_device), ERROR_NONE);
    CHECK_NE(driver_bind(&destroy_driver, &failing_device), ERROR_NONE);
    CHECK_FALSE(driver_is_in_use(&destroy_driver));

    CHECK_EQ(device_remove(&failing_device), ERROR_NONE);
    CHECK_EQ(device_destruct(&failing_device), ERROR_NONE);
    CHECK_EQ(device_remove(&second_device), ERROR_NONE);
    CHECK_EQ(device_destruct(&second_device), ERROR_NONE);
    CHECK_EQ(device_remove(&first_device), ERROR_NONE);
    CHECK_EQ(device_destruct(&first_device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&destroy_driver), ERROR_NONE);
}
