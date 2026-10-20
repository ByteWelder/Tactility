#include "doctest.h"
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/wifi.h>

#include <cstring>
#include <string>

static int access_point_start_calls = 0;
static int access_point_stop_calls = 0;
static WifiAccessPointConfig last_config {};

static error_t fake_access_point_start(Device*, const WifiAccessPointConfig* config) {
    access_point_start_calls++;
    last_config = *config;
    return ERROR_NONE;
}

static error_t fake_access_point_stop(Device*) {
    access_point_stop_calls++;
    return ERROR_INVALID_STATE;
}

static error_t fake_access_point_get_ipv4_address(Device*, char* ipv4) {
    strcpy(ipv4, "192.168.4.1");
    return ERROR_NONE;
}

// A wifi driver without access point mode: the optional members are left out
static const WifiApi station_only_wifi_api = {};

static Driver station_only_wifi_driver = {
    .name = "wifi_test_station_only_driver",
    .compatible = (const char*[]) { "wifi_test,station_only", nullptr },
    .start_device = nullptr,
    .stop_device = nullptr,
    .api = &station_only_wifi_api,
    .device_type = &WIFI_TYPE,
    .internal = nullptr,
};

static const WifiApi access_point_wifi_api = {
    .access_point_start = fake_access_point_start,
    .access_point_stop = fake_access_point_stop,
    .access_point_get_ipv4_address = fake_access_point_get_ipv4_address,
};

static Driver access_point_wifi_driver = {
    .name = "wifi_test_access_point_driver",
    .compatible = (const char*[]) { "wifi_test,access_point", nullptr },
    .start_device = nullptr,
    .stop_device = nullptr,
    .api = &access_point_wifi_api,
    .device_type = &WIFI_TYPE,
    .internal = nullptr,
};

TEST_CASE("wifi access point functions return ERROR_NOT_SUPPORTED for a driver without access point mode") {
    static Device device {
        .name = "wifi_test_station_only_device",
        .config = nullptr,
        .parent = nullptr,
    };

    CHECK_EQ(driver_construct_add(&station_only_wifi_driver), ERROR_NONE);
    CHECK_EQ(device_construct_add(&device, "wifi_test,station_only"), ERROR_NONE);
    CHECK_EQ(device_start(&device), ERROR_NONE);

    WifiAccessPointConfig config {};
    strcpy(config.ssid, "test");
    char ipv4[16] = {};
    CHECK_EQ(wifi_access_point_start(&device, &config), ERROR_NOT_SUPPORTED);
    CHECK_EQ(wifi_access_point_stop(&device), ERROR_NOT_SUPPORTED);
    CHECK_EQ(wifi_access_point_get_ipv4_address(&device, ipv4), ERROR_NOT_SUPPORTED);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&station_only_wifi_driver), ERROR_NONE);
}

TEST_CASE("wifi access point functions call the driver and return its result") {
    access_point_start_calls = 0;
    access_point_stop_calls = 0;
    last_config = {};

    static Device device {
        .name = "wifi_test_access_point_device",
        .config = nullptr,
        .parent = nullptr,
    };

    CHECK_EQ(driver_construct_add(&access_point_wifi_driver), ERROR_NONE);
    CHECK_EQ(device_construct_add(&device, "wifi_test,access_point"), ERROR_NONE);
    CHECK_EQ(device_start(&device), ERROR_NONE);

    WifiAccessPointConfig config {};
    strcpy(config.ssid, "Tactility-1234");
    strcpy(config.password, "secret-password");
    config.channel = 6;
    config.max_connections = 2;
    CHECK_EQ(wifi_access_point_start(&device, &config), ERROR_NONE);
    CHECK_EQ(access_point_start_calls, 1);
    CHECK_EQ(std::string(last_config.ssid), "Tactility-1234");
    CHECK_EQ(std::string(last_config.password), "secret-password");
    CHECK_EQ(last_config.channel, 6);
    CHECK_EQ(last_config.max_connections, 2);

    char ipv4[16] = {};
    CHECK_EQ(wifi_access_point_get_ipv4_address(&device, ipv4), ERROR_NONE);
    CHECK_EQ(std::string(ipv4), "192.168.4.1");

    // The result of the driver is passed on
    CHECK_EQ(wifi_access_point_stop(&device), ERROR_INVALID_STATE);
    CHECK_EQ(access_point_stop_calls, 1);

    CHECK_EQ(device_stop(&device), ERROR_NONE);
    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    CHECK_EQ(driver_remove_destruct(&access_point_wifi_driver), ERROR_NONE);
}
