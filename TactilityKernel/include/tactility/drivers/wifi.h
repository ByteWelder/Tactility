#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/task_event_group.h>
#include <tactility/error.h>
#include <tactility/firmware/firmware.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Device;

enum WifiAuthenticationType {
    WIFI_AUTHENTICATION_TYPE_OPEN = 0,
    WIFI_AUTHENTICATION_TYPE_WEP,
    WIFI_AUTHENTICATION_TYPE_WPA_PSK,
    WIFI_AUTHENTICATION_TYPE_WPA2_PSK,
    WIFI_AUTHENTICATION_TYPE_WPA_WPA2_PSK,
    WIFI_AUTHENTICATION_TYPE_WPA2_ENTERPRISE,
    WIFI_AUTHENTICATION_TYPE_WPA3_PSK,
    WIFI_AUTHENTICATION_TYPE_WPA2_WPA3_PSK,
    WIFI_AUTHENTICATION_TYPE_WAPI_PSK,
    WIFI_AUTHENTICATION_TYPE_OWE,
    WIFI_AUTHENTICATION_TYPE_WPA3_ENT_192,
    WIFI_AUTHENTICATION_TYPE_WPA3_EXT_PSK,
    WIFI_AUTHENTICATION_TYPE_WPA3_EXT_PSK_MIXED_MODE,
    WIFI_AUTHENTICATION_TYPE_MAX
};

struct WifiApRecord {
    char ssid[33]; // 32 bytes + null terminator
    int8_t rssi;
    int32_t channel;
    enum WifiAuthenticationType authentication_type;
};

enum WifiRadioState {
    WIFI_RADIO_STATE_OFF,
    WIFI_RADIO_STATE_ON_PENDING,
    WIFI_RADIO_STATE_ON,
    WIFI_RADIO_STATE_OFF_PENDING,
};

enum WifiStationState {
    WIFI_STATION_STATE_DISCONNECTED,
    WIFI_STATION_STATE_CONNECTION_PENDING,
    WIFI_STATION_STATE_CONNECTED
};

enum WifiAccessPointState {
    WIFI_ACCESS_POINT_STATE_STARTED,
    WIFI_ACCESS_POINT_STATE_STOPPED,
};

enum WifiEventType {
    /** Radio state changed */
    WIFI_EVENT_TYPE_RADIO_STATE_CHANGED,
    /** WifiStationState changed */
    WIFI_EVENT_TYPE_STATION_STATE_CHANGED,
    /** WifiAccessPointState changed */
    WIFI_EVENT_TYPE_STATION_CONNECTION_RESULT,
    /** WifiAccessPointState changed */
    WIFI_EVENT_TYPE_ACCESS_POINT_STATE_CHANGED,
    /** Started scanning for access points */
    WIFI_EVENT_TYPE_SCAN_STARTED,
    /** Finished scanning for access points */
    WIFI_EVENT_TYPE_SCAN_FINISHED,
};

enum WifiStationConnectionError {
    WIFI_STATION_CONNECTION_ERROR_NONE,
    /** Wrong password */
    WIFI_STATION_CONNECTION_ERROR_WRONG_CREDENTIALS,
    /** Failed to connect in a timely manner */
    WIFI_STATION_CONNECTION_ERROR_TIMEOUT,
    /** SSID not found */
    WIFI_STATION_CONNECTION_ERROR_TARGET_NOT_FOUND,
};

struct WifiEvent {
    enum WifiEventType type;
    union {
        enum WifiRadioState radio_state;
        enum WifiStationState station_state;
        enum WifiAccessPointState access_point_state;
        enum WifiStationConnectionError connection_error;
    };
};

/** Number of events a WifiEventSubscription can hold before wifi_event_emit() starts dropping
 * the newest event for it (still delivered to any other matching subscription). Generous: a
 * device fires these serially, one at a time, not in genuinely concurrent bursts. */
#define WIFI_EVENT_QUEUE_CAPACITY 4

/**
 * Caller-owned subscription node, registered with wifi_event_subscribe() and polled with
 * wifi_event_poll(). Like app_event, this queues events by value (FIFO) rather than
 * coalescing to the latest one: a burst of distinct WifiEventTypes (e.g.
 * WIFI_EVENT_TYPE_STATION_STATE_CHANGED immediately followed by
 * WIFI_EVENT_TYPE_STATION_CONNECTION_RESULT) must each be delivered, not just "something
 * changed."
 * @warning Fields other than `bit` are for internal use only; do not read or write them
 * directly.
 */
struct WifiEventSubscription {
    /** Set by wifi_event_subscribe(). Read-only for the caller: OR it into a
     * task_event_group_wait() mask (alongside other subscriptions sharing the same
     * `event_group`) to block on this subscription and other event sources with one call. */
    uint32_t bit;

    struct {
        /** Caller-owned, borrowed; set by wifi_event_subscribe(). */
        struct TaskEventGroup* event_group;
        /** Guards `queue`/`head`/`count`/`closed` between wifi_event_emit() (driver thread) and
         * wifi_event_poll() (caller's thread) - the two live in different translation units with
         * no other shared lock. */
        struct Mutex ring_mutex;
        struct WifiEvent queue[WIFI_EVENT_QUEUE_CAPACITY];
        uint8_t head;
        uint8_t count;
        /** Set under `ring_mutex` when the device is torn down while still subscribed (see
         * esp32_wifi's stop_device()). `ring_mutex` stays constructed - only the subscription's
         * own wifi_event_unsubscribe() destructs it, since that can't race its own polling. */
        bool closed;

        struct WifiEventSubscription* next;
    } internal;
};

struct WifiApi {
    /**
     * Turn the radio on. Unlike start_device()/stop_device() (which only allocate/free the
     * driver's bookkeeping, so event subscribers can stay subscribed across radio toggles),
     * this is what actually brings the hardware up.
     * @param[in] device the wifi device
     * @return ERROR_NONE on success, or if the radio is already on
     */
    error_t (*set_radio_on)(struct Device* device);

    /**
     * Turn the radio off. See set_radio_on().
     * @param[in] device the wifi device
     * @return ERROR_NONE on success, or if the radio is already off
     */
    error_t (*set_radio_off)(struct Device* device);

    /**
     * Get the radio state of the device.
     * @param[in] device the wifi device
     * @param[out] state the radio state
     * @return ERROR_NONE on success
     */
    error_t (*get_radio_state)(struct Device* device, enum WifiRadioState* state);

    /**
     * Get the station state of the device.
     * @param[in] device the wifi device
     * @param[out] state the station state
     * @return ERROR_NONE on success
     */
    error_t (*get_station_state)(struct Device* device, enum WifiStationState* state);

    /**
     * Get the access point state of the device.
     * @param[in] device the wifi device
     * @param[out] state the access point state
     * @return ERROR_NONE on success
     */
    error_t (*get_access_point_state)(struct Device* device, enum WifiAccessPointState* state);

    /**
     * Check if the device is currently scanning for access points.
     * @param[in] device the wifi device
     * @return true when scanning
     */
    bool (*is_scanning)(struct Device* device);

    /**
     * Start a scan for access points.
     * @param[in] device the wifi device
     * @return ERROR_NONE on success
     */
    error_t (*scan)(struct Device* device);

    /**
     * Get the scan results of the device.
     * @param[in] device the wifi device
     * @param[out] results the buffer to store the scan results
     * @param[in, out] num_results the number of scan results: it's first used as input to determine the size of the buffer, and then as output to get the actual number of results
     * @return ERROR_NONE on success
     */
    error_t (*get_scan_results)(struct Device* device, struct WifiApRecord* results, size_t* num_results);

    /**
     * Get the IPv4 address of the device.
     * @param[in] device the device
     * @param[out] ipv4 the buffer to store the IPv4 address (must be at least 16 bytes, will be null-terminated)
     * @return ERROR_NONE on success
     */
    error_t (*station_get_ipv4_address)(struct Device* device, char* ipv4);

    /**
     * Get the SSID of the access point the device is currently connected to.
     * @param[in] device the wifi device
     * @param[out] ssid the buffer to store the SSID (must be at least 33 bytes, will be null-terminated)
     * @return ERROR_NONE on success
     */
    error_t (*station_get_target_ssid)(struct Device* device, char* ssid);

    /**
     * Connect to an access point.
     * @param[in] device the wifi device
     * @param[in] ssid the SSID of the access point
     * @param[in] password the password of the access point
     * @param[in] channel the Wi-Fi channel to connect to (0 means "any" / no preference)
     * @return ERROR_NONE on success
     */
    error_t (*station_connect)(struct Device* device, const char* ssid, const char* password, int32_t channel);

    /**
     * Disconnect from the current access point.
     * @param[in] device the wifi device
     * @return ERROR_NONE on success
     */
    error_t (*station_disconnect)(struct Device* device);

    /**
     * Get the RSSI of the current access point.
     * @param[in] device the wifi device
     * @param[out] rssi the buffer to store the RSSI
     * @return ERROR_NONE on success
     */
    error_t (*station_get_rssi)(struct Device* device, int32_t* rssi);

    /**
     * Register a subscription for this device's WifiEvents.
     * @warning Does not work in ISR context.
     * @param[in] device the wifi device
     * @param[in,out] sub subscription to register; owns the storage, must stay alive (and
     * stationary) until unsubscribed
     * @param[in] event_group caller-owned group to wait on; must outlive @a sub (i.e. be
     * destructed only after event_unsubscribe()). To block for an event, call
     * task_event_group_wait()/task_event_group_wait_any() on this group (OR sub->bit into the
     * mask, or use _wait_any() to include every subscription sharing it), then drain with
     * wifi_event_poll().
     * @retval ERROR_NONE on success
     * @retval ERROR_RESOURCE @a event_group has no free bits left to claim; @a sub was not registered
     * @retval ERROR_INVALID_STATE @a sub is already registered
     */
    error_t (*event_subscribe)(struct Device* device, struct WifiEventSubscription* sub, struct TaskEventGroup* event_group);

    /**
     * Remove a previously registered subscription.
     * @warning Does not work in ISR context.
     * @param[in] device the wifi device
     * @param[in] sub subscription to remove, as passed to event_subscribe()
     * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
     */
    error_t (*event_unsubscribe)(struct Device* device, struct WifiEventSubscription* sub);

    /**
     * Get this device's co-processor firmware update interface, if it has one.
     * @param[in] device the wifi device
     * @param[out] ops filled in with the FirmwareOps vtable and its ctx, if supported
     * @return ERROR_NONE on success, ERROR_NOT_SUPPORTED if this device has no updatable
     * co-processor (ops is left untouched in that case)
     */
    error_t (*get_firmware_ops)(struct Device* device, const struct FirmwareOps** ops, void** ctx);
};

extern const struct DeviceType WIFI_TYPE;

/** Turn the radio on. See WifiApi::set_radio_on(). Requires the device to be started (device_start()). */
error_t wifi_set_radio_on(struct Device* device);
/** Turn the radio off. See WifiApi::set_radio_off(). Requires the device to be started (device_start()). */
error_t wifi_set_radio_off(struct Device* device);

error_t wifi_get_radio_state(struct Device* device, enum WifiRadioState* state);
error_t wifi_get_station_state(struct Device* device, enum WifiStationState* state);
error_t wifi_get_access_point_state(struct Device* device, enum WifiAccessPointState* state);
bool wifi_is_scanning(struct Device* device);
error_t wifi_scan(struct Device* device);
error_t wifi_get_scan_results(struct Device* device, struct WifiApRecord* results, size_t* num_results);
error_t wifi_station_get_ipv4_address(struct Device* device, char* ipv4);
error_t wifi_station_get_target_ssid(struct Device* device, char* ssid);
error_t wifi_station_connect(struct Device* device, const char* ssid, const char* password, int32_t channel);
error_t wifi_station_disconnect(struct Device* device);
error_t wifi_station_get_rssi(struct Device* device, int32_t* rssi);

/**
 * Register a subscription for @a device's WifiEvents.
 * @warning Does not work in ISR context.
 * @param[in] device the wifi device
 * @param[in,out] sub subscription to register; owns the storage, must stay alive (and
 * stationary) until unsubscribed
 * @param[in] event_group caller-owned group to wait on; must outlive @a sub. To block for an
 * event, call task_event_group_wait()/task_event_group_wait_any() on this group (OR sub->bit
 * into the mask, or use _wait_any() to include every subscription sharing it), then drain with
 * wifi_event_poll().
 * @retval ERROR_NONE on success
 * @retval ERROR_RESOURCE @a event_group has no free bits left to claim; @a sub was not registered
 * @retval ERROR_INVALID_STATE @a sub is already registered
 */
error_t wifi_event_subscribe(struct Device* device, struct WifiEventSubscription* sub, struct TaskEventGroup* event_group);

/**
 * Remove a previously registered subscription.
 * @warning Does not work in ISR context.
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
 */
error_t wifi_event_unsubscribe(struct Device* device, struct WifiEventSubscription* sub);

/**
 * Non-blocking: pop the next event for @a sub if one is already queued.
 * @warning Never blocks. To wait for an event, block in task_event_group_wait()/
 * task_event_group_wait_any() on @a sub's event group first (see wifi_event_subscribe()), then
 * drain with this in a loop.
 * @retval ERROR_NONE @a out_event was filled
 * @retval ERROR_TIMEOUT nothing queued right now
 */
error_t wifi_event_poll(struct WifiEventSubscription* sub, struct WifiEvent* out_event);

error_t wifi_get_firmware_ops(struct Device* device, const struct FirmwareOps** ops, void** ctx);

#ifdef __cplusplus
}
#endif
