// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <tactility/error.h>

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
 * @param[in] context the context pointer passed to system_event_subscribe()
 */
typedef void (*system_event_callback_t)(struct SystemEvent* event, void* context);

/**
 * Subscribe to system events of a given type.
 * @warning Does not work in ISR context.
 * @warning @a callback is invoked synchronously, on the caller's task, from within
 * system_event_emit(). The internal subscription lock is not held during the call, so
 * @a callback may itself call system_event_subscribe(), system_event_unsubscribe() or
 * system_event_emit() without deadlocking - but a subscribe/unsubscribe made from within
 * a callback only takes effect for events emitted after the current system_event_emit()
 * call returns, since that call already snapshotted the subscriptions it will invoke.
 * @param[in] type the event type to subscribe to
 * @param[in] callback the callback to invoke when a matching event is emitted
 * @param[in] context an opaque pointer passed back to @a callback unmodified
 * @return ERROR_NONE on success
 */
error_t system_event_subscribe(
    enum SystemEventType type,
    system_event_callback_t callback,
    void *context
);

/**
 * Remove a previously added subscription.
 * @warning Does not work in ISR context.
 * @param[in] type the event type passed to the matching system_event_subscribe() call
 * @param[in] callback the callback passed to the matching system_event_subscribe() call
 * @return ERROR_NONE on success, ERROR_NOT_FOUND if no matching subscription exists
 */
error_t system_event_unsubscribe(
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

#ifdef __cplusplus
}
#endif
