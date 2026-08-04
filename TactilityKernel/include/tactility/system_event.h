// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <tactility/error.h>
#include <tactility/freertos/freertos.h>
#include <tactility/freertos/task.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Device;
struct FileSystem;

/** Identifies a system-wide event */
enum SystemEventType {
    KERNEL_EVENT_BOOT_COMPLETED, // No data
    KERNEL_EVENT_NETWORK_CONNECTED, // struct NetworkConnectedEvent
    KERNEL_EVENT_NETWORK_DISCONNECTED, // struct NetworkDisconnectedEvent
    KERNEL_EVENT_FILE_SYSTEM_MOUNTED, // struct FileSystemMountedEvent
    KERNEL_EVENT_FILE_SYSTEM_UNMOUNTED, // struct FileSystemUnmountedEvent
    KERNEL_EVENT_SERVICE_STARTED, // ServiceStartedEvent
    KERNEL_EVENT_SERVICE_STOPPED, // ServiceStoppedEvent
    KERNEL_EVENT_TIME_CHANGED, // No data - fired whenever system time is set (NTP sync, RTC restore, manual change)
};

/**
 * A system-wide event as delivered to a system_event_callback_t.
 * `data` points at the type-specific struct documented next to `type`'s enum value
 * in SystemEventType (or is NULL when none is documented).
 * It is only valid for the duration of the callback.
 */
struct SystemEvent {
    enum SystemEventType type;
    /** Microseconds since boot, from get_micros_since_boot(). */
    uint64_t timestamp;
    const void *data;
    size_t data_len;
};

/** Data for KERNEL_EVENT_NETWORK_CONNECTED. */
struct NetworkConnectedEvent {
    struct Device* device;
    uint32_t ipv4_addr;
    uint32_t gateway;
};

/** Data for KERNEL_EVENT_NETWORK_DISCONNECTED. */
struct NetworkDisconnectedEvent {
    struct Device* device;
};

/** Data for KERNEL_EVENT_FILE_SYSTEM_MOUNTED. */
struct FileSystemMountedEvent {
    struct FileSystem* file_system;
};

/** Data for KERNEL_EVENT_FILE_SYSTEM_UNMOUNTED. */
struct FileSystemUnmountedEvent {
    struct FileSystem* file_system;
};

/** Data for KERNEL_EVENT_SERVICE_STARTED. */
struct ServiceStartedEvent {
    const char* id;
};

/** Data for KERNEL_EVENT_SERVICE_STOPPED. */
struct ServiceStoppedEvent {
    const char* id;
};

/**
 * @param[in] event the event being delivered; only valid for the duration of the call
 * @param[in] context the context pointer passed to system_event_callback_add()
 */
typedef void (*system_event_callback_t)(struct SystemEvent* event, void* context);

/**
 * Subscribe to system events of a given type.
 * @warning Does not work in ISR context.
 * @warning @a callback is invoked synchronously, on the caller's task, from within
 * system_event_emit(). The internal subscription lock is not held during the call, so
 * @a callback may itself call system_event_callback_add(), system_event_callback_remove() or
 * system_event_emit() without deadlocking - but a subscribe/unsubscribe made from within
 * a callback only takes effect for events emitted after the current system_event_emit()
 * call returns, since that call already snapshotted the subscriptions it will invoke.
 * @param[in] type the event type to subscribe to
 * @param[in] callback the callback to invoke when a matching event is emitted
 * @param[in] context an opaque pointer passed back to @a callback unmodified
 * @return ERROR_NONE on success
 */
error_t system_event_callback_add(
    enum SystemEventType type,
    system_event_callback_t callback,
    void *context
);

/**
 * Remove a previously added subscription.
 * @warning Does not work in ISR context.
 * @param[in] type the event type passed to the matching system_event_callback_add() call
 * @param[in] callback the callback passed to the matching system_event_callback_add() call
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
 */
error_t system_event_callback_remove(
    enum SystemEventType type,
    system_event_callback_t callback
);

/**
 * Emit a system event, synchronously invoking every subscription registered for @a type
 * (in subscription order) on the calling task before returning.
 * @warning Does not work in ISR context.
 * @param[in] type the event type
 * @param[in] data optional pointer to the type-specific event struct (see SystemEventType);
 * only valid for the duration of this call, subscribers must not retain it
 * @param[in] data_len size of @a data in bytes (0 if @a data is NULL)
 * @return ERROR_NONE on success
 */
error_t system_event_emit(
    enum SystemEventType type,
    const void* data,
    size_t data_len
);

/** Size of the largest type-specific event struct documented in SystemEventType, i.e. the
 * embedded buffer size needed by SystemEventSubscription to hold any event's payload by value. */
#define SYSTEM_EVENT_MAX_DATA_SIZE (sizeof(struct NetworkConnectedEvent))

/**
 * gps.h-style poll subscription: caller-owned node, registered with system_event_subscribe()
 * and polled with system_event_await(). Unlike system_event_callback_t, the payload is copied
 * by value into @a data (up to SYSTEM_EVENT_MAX_DATA_SIZE bytes) so it remains valid after
 * system_event_emit() returns.
 * @warning Fields other than `type` are for internal use only; do not read or write them
 * directly.
 */
struct SystemEventSubscription {
    /** Event type to subscribe to; set by the caller before system_event_subscribe(). */
    enum SystemEventType type;

    TaskHandle_t task;

    uint64_t timestamp;
    uint8_t data[SYSTEM_EVENT_MAX_DATA_SIZE];
    size_t data_len;

    uint32_t sequence;
    uint32_t consumed_sequence;

    struct SystemEventSubscription* next;
};

/**
 * Register a poll subscription for events of @a sub->type.
 * @warning Does not work in ISR context.
 * @param[in,out] sub subscription to register; caller sets @a sub->type beforehand, owns the
 * storage, and must keep it alive (and stationary) until unsubscribed
 * @return ERROR_NONE on success
 */
error_t system_event_subscribe(struct SystemEventSubscription* sub);

/**
 * Remove a previously registered poll subscription.
 * @warning Does not work in ISR context.
 * @param[in] sub subscription to remove, as passed to system_event_subscribe()
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
 */
error_t system_event_unsubscribe(struct SystemEventSubscription* sub);

/**
 * Blocks the calling task until a new event arrives for @a sub, or timeout elapses.
 * @param[in,out] sub subscription to wait on, as passed to system_event_subscribe()
 * @param[in] timeout max ticks to wait
 * @return ERROR_NONE if an event arrived, ERROR_TIMEOUT if the timeout elapsed
 */
error_t system_event_await(struct SystemEventSubscription* sub, TickType_t timeout);

#ifdef __cplusplus
}
#endif
