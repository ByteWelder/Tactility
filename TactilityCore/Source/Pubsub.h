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
 * 
 * Threadsafe, Reentrable
 * 
 * @param      pubsub            pointer to PubSub instance
 * @param[in]  callback          The callback
 * @param      callback_context  The callback context
 *
 * @return     pointer to PubSubSubscription instance
 */
PubSubSubscription*
tt_pubsub_subscribe(std::shared_ptr<PubSub> pubsub, PubSubCallback callback, void* callback_context);

/** Unsubscribe from PubSub
 * 
 * No use of `pubsub_subscription` allowed after call of this method
 * Threadsafe, Reentrable.
 *
 * @param      pubsub               pointer to PubSub instance
 * @param      pubsub_subscription  pointer to PubSubSubscription instance
 */
void tt_pubsub_unsubscribe(std::shared_ptr<PubSub> pubsub, PubSubSubscription* pubsub_subscription);

/** Publish message to PubSub
 *
 * Threadsafe, Reentrable.
 * 
 * @param      pubsub   pointer to PubSub instance
 * @param      message  message pointer to publish
 */
void tt_pubsub_publish(std::shared_ptr<PubSub> pubsub, void* message);

} // namespace
