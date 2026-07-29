#include <tactility/system_event.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>
#include <tactility/time.h>

#include <algorithm>
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

extern "C" {

error_t system_event_subscribe(
    SystemEventType type,
    system_event_callback_t callback,
    void* context
) {
    mutex_lock(&subscriptions_mutex.handle);
    subscriptions.push_back(KernelEventSubscription { type, callback, context });
    mutex_unlock(&subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t system_event_unsubscribe(
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

error_t system_event_emit(
    enum SystemEventType type,
    const void* data,
    size_t data_len
) {
    SystemEvent event = {
        .type = type,
        .timestamp = get_micros_since_boot(),
        .data = data,
        .data_len = data_len,
    };

    // Snapshot matching subscriptions under the lock, then invoke after unlocking: a
    // callback calling system_event_subscribe(), system_event_unsubscribe() or
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
        if (subscription.type == type) {
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
        if (subscription.type == type) {
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

} // extern "C"
