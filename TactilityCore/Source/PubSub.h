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

/** Publish and subscribe to messages in a thread-safe manner. */
class PubSub {

private:

    struct Subscription {
        uint64_t id;
        PubSubCallback callback;
        void* callbackParameter;
    };

    typedef std::list<Subscription> Subscriptions;
    uint64_t lastId = 0;
    Subscriptions items;
    Mutex mutex;

public:

    typedef void* SubscriptionHandle;

    PubSub() = default;

    ~PubSub() {
        tt_check(items.empty());
    }

    /** Start receiving messages at the specified handle (Threadsafe, Re-entrable)
     * @param[in] callback
     * @param[in] callbackParameter the data to pass to the callback
     * @return subscription instance
     */
    SubscriptionHandle subscribe(PubSubCallback callback, void* callbackParameter);

    /** Stop receiving messages at the specified handle (Threadsafe, Re-entrable.)
     * No use of `tt_pubsub_subscription` allowed after call of this method
     * @param[in] subscription
     */
    void unsubscribe(SubscriptionHandle subscription);

    /** Publish message to all subscribers (Threadsafe, Re-entrable.)
     * @param[in] message message pointer to publish - it is passed as-is to the callback
     */
    void publish(void* message);
};


} // namespace
