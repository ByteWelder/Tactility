#include <tactility/system_event.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>
#include <tactility/time.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

struct KernelEventSubscription {
    SystemEventType type;
    system_event_callback_t callback;
    void* callback_context;
};

static std::vector<KernelEventSubscription> subscriptions;

// Mutex is constructed/destructed via a static-lifetime wrapper because struct Mutex
// itself has no constructor: mutex_lock() on an unconstructed handle is undefined
// behaviour (the raw QueueHandle_t would be null).
struct KernelEventMutex {
    Mutex handle {};
    KernelEventMutex() { mutex_construct(&handle); }
    ~KernelEventMutex() { mutex_destruct(&handle); }
};

static KernelEventMutex subscriptions_mutex;

// Intrusive singly-linked list of poll subscriptions (system_event_subscribe()/_unsubscribe()/
// _await()), separate from the callback-based `subscriptions` vector above. Guarded by its own
// mutex since notifying a poll subscriber never invokes caller code (just a memcpy and an
// xTaskNotifyGive), so there is no reentrancy concern requiring a snapshot-then-unlock dance.
static SystemEventSubscription* poll_subscriptions = nullptr;
static KernelEventMutex poll_subscriptions_mutex;

extern "C" {

error_t system_event_callback_add(
    SystemEventType type,
    system_event_callback_t callback,
    void* context
) {
    mutex_lock(&subscriptions_mutex.handle);
    subscriptions.push_back(KernelEventSubscription { type, callback, context });
    mutex_unlock(&subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t system_event_callback_remove(
    SystemEventType type,
    system_event_callback_t callback
) {
    mutex_lock(&subscriptions_mutex.handle);
    const auto iterator = std::ranges::find_if(subscriptions, [type, callback](const KernelEventSubscription& subscription) {
        return subscription.type == type && subscription.callback == callback;
    });
    error_t result = ERROR_NOT_FOUND;
    if (iterator != subscriptions.end()) {
        subscriptions.erase(iterator);
        result = ERROR_NONE;
    }
    mutex_unlock(&subscriptions_mutex.handle);

    return result;
}

// Copies `data` into every current poll subscriber of `type` and wakes its waiting task.
// Held entirely under the lock: unlike the callback path, this never invokes caller code
// (just a memcpy and an xTaskNotifyGive), so there is nothing that could reenter and deadlock.
static void notify_poll_subscribers(
    SystemEventType type,
    uint64_t timestamp,
    const void* data,
    size_t data_len
) {
    mutex_lock(&poll_subscriptions_mutex.handle);

    for (SystemEventSubscription* sub = poll_subscriptions; sub != nullptr; sub = sub->next) {
        if (sub->type == type) {
            sub->timestamp = timestamp;
            if (data_len > 0) {
                std::memcpy(sub->data, data, std::min(data_len, static_cast<size_t>(SYSTEM_EVENT_MAX_DATA_SIZE)));
            }
            sub->data_len = data_len;
            sub->sequence++;
            xTaskNotifyGive(sub->task);
        }
    }

    mutex_unlock(&poll_subscriptions_mutex.handle);
}

static error_t notify_listeners(
    SystemEvent& event
) {
    // Snapshot matching subscriptions under the lock, then invoke after unlocking: a
    // callback calling system_event_callback_add(), system_event_callback_remove() or
    // system_event_emit() would otherwise deadlock against this same (non-recursive)
    // mutex, and a slow callback would block every other thread's subscribe/unsubscribe
    // for the duration of this emit.
    //
    // Nothing between mutex_lock() and mutex_unlock() below may throw.
    // Count first, then use new(std::nothrow) to allocate the exact size and report
    // failure through the return value instead; the fill loop below is then a plain
    // assignment of a trivially-copyable struct, which cannot throw or reallocate.

    mutex_lock(&subscriptions_mutex.handle);

    size_t match_count = 0;
    for (const auto& subscription : subscriptions) {
        if (subscription.type == event.type) {
            match_count++;
        }
    }

    KernelEventSubscription* matching = (match_count > 0)
        ? new (std::nothrow) KernelEventSubscription[match_count]
        : nullptr;
    if (match_count > 0 && matching == nullptr) {
        mutex_unlock(&subscriptions_mutex.handle);
        return ERROR_OUT_OF_MEMORY;
    }

    size_t matched_count = 0;
    for (const auto& subscription : subscriptions) {
        if (subscription.type == event.type) {
            matching[matched_count++] = subscription;
        }
    }

    mutex_unlock(&subscriptions_mutex.handle);

    for (size_t i = 0; i < matched_count; i++) {
        matching[i].callback(&event, matching[i].callback_context);
    }

    delete[] matching;

    return ERROR_NONE;
}

error_t system_event_emit(
    SystemEventType type,
    const void* data,
    size_t data_len
) {
    SystemEvent event = {
        .type = type,
        .timestamp = get_micros_since_boot(),
        .data = data,
        .data_len = data_len,
    };

    notify_poll_subscribers(type, event.timestamp, data, data_len);
    auto error = notify_listeners(event);
    if (error != ERROR_NONE) { return error; }

    return ERROR_NONE;
}

error_t system_event_subscribe(SystemEventSubscription* sub) {
    sub->task = xTaskGetCurrentTaskHandle();
    sub->sequence = 0;
    sub->consumed_sequence = 0;
    sub->data_len = 0;

    mutex_lock(&poll_subscriptions_mutex.handle);
    sub->next = poll_subscriptions;
    poll_subscriptions = sub;
    mutex_unlock(&poll_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t system_event_unsubscribe(SystemEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&poll_subscriptions_mutex.handle);
    for (SystemEventSubscription** link = &poll_subscriptions; *link != nullptr; link = &(*link)->next) {
        if (*link == sub) {
            *link = sub->next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&poll_subscriptions_mutex.handle);

    return result;
}

error_t system_event_await(SystemEventSubscription* sub, TickType_t timeout) {
    uint32_t old_sequence = sub->sequence;

    while (sub->sequence == old_sequence) {
        if (ulTaskNotifyTake(pdTRUE, timeout) == 0) {
            return ERROR_TIMEOUT;
        }
    }

    sub->consumed_sequence = sub->sequence;
    return ERROR_NONE;
}

} // extern "C"
