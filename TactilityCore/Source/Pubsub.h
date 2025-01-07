/**
 * @file pubsub.h
 * PubSub
 */
#pragma once

#include "Mutex.h"
#include <list>

namespace tt {

/** PubSub Callback type */
typedef void (*PubSubCallback)(const void* message, void* context);

struct PubSubSubscription {
    uint64_t id;
    PubSubCallback callback;
    void* callback_context;
};

struct PubSub {
    typedef std::list<PubSubSubscription> Subscriptions;
    uint64_t last_id = 0;
    Subscriptions items;
    Mutex mutex;

    ~PubSub() {
        tt_check(items.empty());
    }
};

/** Subscribe to PubSub
 * Threadsafe, Reentrable
 * @param[in] pubsub pointer to PubSub instance
 * @param[in] callback
 * @param[in] callbackContext the data to pass to the callback
 * @return subscription instance
 */
PubSubSubscription*
tt_pubsub_subscribe(std::shared_ptr<PubSub> pubsub, PubSubCallback callback, void* callbackContext);

/** Unsubscribe from PubSub
 * No use of `tt_pubsub_subscription` allowed after call of this method
 * Threadsafe, Re-entrable.
 * @param[in] pubsub
 * @param[in] subscription
 */
void tt_pubsub_unsubscribe(std::shared_ptr<PubSub> pubsub, PubSubSubscription* subscription);

/** Publish message to PubSub
 * Threadsafe, Reentrable.
 * @param[in] pubsub
 * @param[in] message message pointer to publish - it is passed as-is to the callback
 */
void tt_pubsub_publish(std::shared_ptr<PubSub> pubsub, void* message);

} // namespace
