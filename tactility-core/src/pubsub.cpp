#include "pubsub.h"
#include "check.h"
#include "Mutex.h"
#include <list>


struct PubSubSubscription {
    uint64_t id;
    PubSubCallback callback;
    void* callback_context;
};

typedef std::list<PubSubSubscription> Subscriptions;

struct PubSub {
    uint64_t last_id = 0;
    Subscriptions items;
    Mutex* mutex;
};

PubSub* tt_pubsub_alloc() {
    auto* pubsub = new PubSub();

    pubsub->mutex = tt_mutex_alloc(MutexTypeNormal);
    tt_assert(pubsub->mutex);

    return pubsub;
}

void tt_pubsub_free(PubSub* pubsub) {
    tt_assert(pubsub);
    tt_check(pubsub->items.empty());
    tt_mutex_free(pubsub->mutex);
    delete pubsub;
}

PubSubSubscription* tt_pubsub_subscribe(PubSub* pubsub, PubSubCallback callback, void* callback_context) {
    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);
    PubSubSubscription subscription = {
        .id = (++pubsub->last_id),
        .callback = callback,
        .callback_context = callback_context
    };
    pubsub->items.push_back(
        subscription
    );

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);

    return (PubSubSubscription*)pubsub->last_id;
}

void tt_pubsub_unsubscribe(PubSub* pubsub, PubSubSubscription* pubsub_subscription) {
    tt_assert(pubsub);
    tt_assert(pubsub_subscription);

    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);
    bool result = false;
    auto id = (uint64_t)pubsub_subscription;
    for (auto it = pubsub->items.begin(); it != pubsub->items.end(); it++) {
        if (it->id == id) {
            pubsub->items.erase(it);
            result = true;
            break;
        }
    }

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);
    tt_check(result);
}

void tt_pubsub_publish(PubSub* pubsub, void* message) {
    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);

    // Iterate over subscribers
    for (auto& it : pubsub->items) {
        it.callback(message, it.callback_context);
    }

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);
}
