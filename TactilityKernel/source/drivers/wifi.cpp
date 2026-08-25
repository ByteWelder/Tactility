// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/wifi.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define WIFI_API(device) ((const struct WifiApi*)device_get_driver(device)->api)

extern "C" {

error_t wifi_set_radio_on(struct Device* device) {
    return WIFI_API(device)->set_radio_on(device);
}

error_t wifi_set_radio_off(struct Device* device) {
    return WIFI_API(device)->set_radio_off(device);
}

error_t wifi_get_radio_state(struct Device* device, enum WifiRadioState* state) {
    return WIFI_API(device)->get_radio_state(device, state);
}

error_t wifi_get_station_state(struct Device* device, enum WifiStationState* state) {
    return WIFI_API(device)->get_station_state(device, state);
}

error_t wifi_get_access_point_state(struct Device* device, enum WifiAccessPointState* state) {
    return WIFI_API(device)->get_access_point_state(device, state);
}

bool wifi_is_scanning(struct Device* device) {
    return WIFI_API(device)->is_scanning(device);
}

error_t wifi_scan(struct Device* device) {
    return WIFI_API(device)->scan(device);
}

error_t wifi_get_scan_results(struct Device* device, struct WifiApRecord* results, size_t* num_results) {
    return WIFI_API(device)->get_scan_results(device, results, num_results);
}

error_t wifi_station_get_ipv4_address(struct Device* device, char* ipv4) {
    return WIFI_API(device)->station_get_ipv4_address(device, ipv4);
}

error_t wifi_station_get_target_ssid(struct Device* device, char* ssid) {
    return WIFI_API(device)->station_get_target_ssid(device, ssid);
}

error_t wifi_station_connect(struct Device* device, const char* ssid, const char* password, int32_t channel) {
    return WIFI_API(device)->station_connect(device, ssid, password, channel);
}

error_t wifi_station_disconnect(struct Device* device) {
    return WIFI_API(device)->station_disconnect(device);
}

error_t wifi_station_get_rssi(struct Device* device, int32_t* rssi) {
    return WIFI_API(device)->station_get_rssi(device, rssi);
}

error_t wifi_event_subscribe(struct Device* device, struct WifiEventSubscription* sub, struct TaskEventGroup* event_group) {
    mutex_construct(&sub->internal.ring_mutex);
    sub->internal.head = 0;
    sub->internal.count = 0;

    error_t result = WIFI_API(device)->event_subscribe(device, sub, event_group);
    if (result != ERROR_NONE) {
        mutex_destruct(&sub->internal.ring_mutex);
    }
    return result;
}

error_t wifi_event_unsubscribe(struct Device* device, struct WifiEventSubscription* sub) {
    error_t result = WIFI_API(device)->event_unsubscribe(device, sub);
    mutex_lock(&sub->internal.ring_mutex);
    bool was_closed = sub->internal.closed;
    mutex_unlock(&sub->internal.ring_mutex);
    // Destruct on success or force-close; skip on a genuine caller error (never subscribed, so
    // ring_mutex was never constructed).
    if (result == ERROR_NONE || was_closed) {
        mutex_destruct(&sub->internal.ring_mutex);
    }
    return result;
}

error_t wifi_event_poll(struct WifiEventSubscription* sub, struct WifiEvent* out_event) {
    mutex_lock(&sub->internal.ring_mutex);
    if (sub->internal.closed) {
        mutex_unlock(&sub->internal.ring_mutex);
        return ERROR_TIMEOUT;
    }
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % WIFI_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&sub->internal.ring_mutex);
    return has_event ? ERROR_NONE : ERROR_TIMEOUT;
}

error_t wifi_get_firmware_ops(struct Device* device, const struct FirmwareOps** ops, void** ctx) {
    auto* api = WIFI_API(device);
    if (api->get_firmware_ops == nullptr) {
        return ERROR_NOT_SUPPORTED;
    }
    return api->get_firmware_ops(device, ops, ctx);
}

const struct DeviceType WIFI_TYPE = {
    .name = "wifi"
};

} // extern "C"
