#include "Pubsub.h"
#include "Check.h"

namespace tt {

PubSubSubscription* tt_pubsub_subscribe(std::shared_ptr<PubSub> pubsub, PubSubCallback callback, void* callbackContext) {
    tt_check(pubsub->mutex.acquire(TtWaitForever) == TtStatusOk);
    PubSubSubscription subscription = {
        .id = (++pubsub->last_id),
        .callback = callback,
        .callback_context = callbackContext
    };
    pubsub->items.push_back(
        subscription
    );

    tt_check(pubsub->mutex.release() == TtStatusOk);

    return (PubSubSubscription*)pubsub->last_id;
}

void tt_pubsub_unsubscribe(std::shared_ptr<PubSub> pubsub, PubSubSubscription* pubsub_subscription) {
    assert(pubsub);
    assert(pubsub_subscription);

    tt_check(pubsub->mutex.acquire(TtWaitForever) == TtStatusOk);
    bool result = false;
    auto id = (uint64_t)pubsub_subscription;
    for (auto it = pubsub->items.begin(); it != pubsub->items.end(); it++) {
        if (it->id == id) {
            pubsub->items.erase(it);
            result = true;
            break;
        }
    }

    tt_check(pubsub->mutex.release() == TtStatusOk);
    tt_check(result);
}

void tt_pubsub_publish(std::shared_ptr<PubSub> pubsub, void* message) {
    tt_check(pubsub->mutex.acquire(TtWaitForever) == TtStatusOk);

    // Iterate over subscribers
    for (auto& it : pubsub->items) {
        it.callback(message, it.callback_context);
    }

    tt_check(pubsub->mutex.release() == TtStatusOk);
}

} // namespace
