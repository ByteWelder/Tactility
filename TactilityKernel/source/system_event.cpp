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
// _poll()), separate from the callback-based `subscriptions` vector above. Guarded by its own
// mutex since notifying a poll subscriber never invokes caller code (just a memcpy and a
// task_event_group_signal), so there is no reentrancy concern requiring a snapshot-then-unlock dance.
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

// Copies `data` into every current poll subscriber of `type` and signals its wakeup bit.
// Held entirely under the lock: unlike the callback path, this never invokes caller code
// (just a memcpy and a signal), so there is nothing that could reenter and deadlock.
static void notify_poll_subscribers(
    SystemEventType type,
    uint64_t timestamp,
    const void* data,
    size_t data_len
) {
    mutex_lock(&poll_subscriptions_mutex.handle);

    for (SystemEventSubscription* sub = poll_subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->event.type == type) {
            sub->event.timestamp = timestamp;
            const size_t copied_len = std::min(data_len, SYSTEM_EVENT_MAX_DATA_SIZE);
            if (copied_len > 0) {
                std::memcpy(sub->event.data, data, copied_len);
            }
            sub->event.data_len = copied_len;
            sub->internal.sequence++;
            task_event_group_signal(sub->internal.event_group, sub->bit);
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
    SystemEvent event {};
    event.type = type;
    event.timestamp = get_micros_since_boot();
    const size_t copied_len = std::min(data_len, SYSTEM_EVENT_MAX_DATA_SIZE);
    if (copied_len > 0) {
        std::memcpy(event.data, data, copied_len);
    }
    event.data_len = copied_len;

    notify_poll_subscribers(type, event.timestamp, data, data_len);
    auto error = notify_listeners(event);
    if (error != ERROR_NONE) { return error; }

    return ERROR_NONE;
}

error_t system_event_subscribe(SystemEventSubscription* sub, TaskEventGroup* event_group) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    mutex_lock(&poll_subscriptions_mutex.handle);

    // Check-and-insert in one critical section: registering the same `sub` twice would link
    // it into a list that already contains it, creating a cycle that notify_poll_subscribers()
    // would then traverse forever while holding this same mutex.
    for (SystemEventSubscription* existing = poll_subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&poll_subscriptions_mutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }

    sub->internal.event_group = event_group;
    sub->bit = bit;
    sub->internal.sequence = 0;
    sub->internal.consumed_sequence = 0;
    sub->internal.cancelled = false;
    sub->event.data_len = 0;
    sub->internal.next = poll_subscriptions;
    poll_subscriptions = sub;

    mutex_unlock(&poll_subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t system_event_unsubscribe(SystemEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&poll_subscriptions_mutex.handle);
    for (SystemEventSubscription** link = &poll_subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    if (result == ERROR_NONE) {
        // Diagnostic only now (see @warning) - not relied on for safety. Best-effort nudge for a
        // task that might still be blocked in task_event_group_wait() on this bit; unlike before,
        // this does not wait for it to leave before the bit is released.
        sub->internal.cancelled = true;
        task_event_group_signal(sub->internal.event_group, sub->bit);
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&poll_subscriptions_mutex.handle);

    return result;
}

error_t system_event_poll(SystemEventSubscription* sub) {
    mutex_lock(&poll_subscriptions_mutex.handle);
    bool pending = sub->internal.sequence != sub->internal.consumed_sequence;
    bool cancelled = sub->internal.cancelled;
    if (pending) {
        sub->internal.consumed_sequence = sub->internal.sequence;
    }
    mutex_unlock(&poll_subscriptions_mutex.handle);

    if (pending) {
        return ERROR_NONE;
    }
    if (cancelled) {
        return ERROR_INVALID_STATE;
    }
    return ERROR_TIMEOUT;
}

error_t system_event_get_data(SystemEventSubscription* sub, uint8_t* data, size_t data_len) {
    // sub->event.* is written by notify_poll_subscribers() under poll_subscriptions_mutex -
    // the length check and the copy must happen as one snapshot under the same lock, otherwise
    // a concurrent emit could grow data_len (or overwrite data) between the check and the
    // memcpy below.
    mutex_lock(&poll_subscriptions_mutex.handle);
    error_t result = ERROR_NONE;
    if (data_len < sub->event.data_len) {
        result = ERROR_BUFFER_OVERFLOW;
    } else {
        std::memcpy(data, sub->event.data, sub->event.data_len);
    }
    mutex_unlock(&poll_subscriptions_mutex.handle);
    return result;
}

} // extern "C"
