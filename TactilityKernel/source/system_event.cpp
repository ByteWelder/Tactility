#include <tactility/system_event.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>
#include <tactility/time.h>

#include <algorithm>
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

    mutex_lock(&subscriptions_mutex.handle);
    std::vector<KernelEventSubscription> matching;
    for (const auto& subscription : subscriptions) {
        if (subscription.type == type) {
            subscription.callback(&event, subscription.callback_context);
        }
    }
    mutex_unlock(&subscriptions_mutex.handle);

    return ERROR_NONE;
}

} // extern "C"
