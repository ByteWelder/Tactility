// SPDX-License-Identifier: Apache-2.0
#include <app/event.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/time.h>

/**
 * Intrusive singly-linked list of subscriptions, keyed by app_instance_id.
 * Guarded by a single coarse-grained mutex, notifying a subscriber here never invokes caller code
 * (just a struct copy and a task_event_group_signal), so there is no reentrancy concern requiring a snapshot-then-unlock dance.
 */
static AppEventSubscription* subscriptions = nullptr;

struct AppEventMutex {
    Mutex handle {};
    AppEventMutex() { mutex_construct(&handle); }
    ~AppEventMutex() { mutex_destruct(&handle); }
};

static AppEventMutex subscriptions_mutex;

extern "C" {

error_t app_event_subscribe(AppEventSubscription* sub, TaskEventGroup* event_group) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    sub->bit = bit;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;

    mutex_lock(&subscriptions_mutex.handle);
    sub->internal.next = subscriptions;
    subscriptions = sub;
    mutex_unlock(&subscriptions_mutex.handle);

    return ERROR_NONE;
}

error_t app_event_unsubscribe(AppEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&subscriptions_mutex.handle);
    for (AppEventSubscription** link = &subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&subscriptions_mutex.handle);

    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }

    return result;
}

error_t app_event_emit(AppInstanceId app_instance_id, const AppEvent* event) {
    AppEvent stamped_event = *event;
    stamped_event.timestamp = get_micros_since_boot();

    error_t result = ERROR_NOT_FOUND;

    mutex_lock(&subscriptions_mutex.handle);
    for (AppEventSubscription* sub = subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->app_instance_id != app_instance_id) {
            continue;
        }

        if (sub->internal.count >= APP_EVENT_QUEUE_CAPACITY) {
            result = ERROR_RESOURCE;
            continue;
        }

        uint8_t tail = (sub->internal.head + sub->internal.count) % APP_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = stamped_event;
        sub->internal.count++;
        if (result != ERROR_RESOURCE) {
            result = ERROR_NONE;
        }
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    mutex_unlock(&subscriptions_mutex.handle);

    return result;
}

static bool try_pop(AppEventSubscription* sub, AppEvent* out_event) {
    mutex_lock(&subscriptions_mutex.handle);
    bool has_event = sub->internal.count > 0;
    if (has_event) {
        *out_event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % APP_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    mutex_unlock(&subscriptions_mutex.handle);
    return has_event;
}

error_t app_event_poll(AppEventSubscription* sub, AppEvent* out_event) {
    return try_pop(sub, out_event) ? ERROR_NONE : ERROR_TIMEOUT;
}

} // extern "C"
