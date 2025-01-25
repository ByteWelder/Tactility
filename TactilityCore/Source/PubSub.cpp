#include "PubSub.h"
#include "Check.h"

namespace tt {

PubSub::SubscriptionHandle PubSub::subscribe(PubSubCallback callback, void* callbackContext) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    items.push_back({
        .id = (++last_id),
        .callback = callback,
        .callback_context = callbackContext
    });

    tt_check(mutex.release() == TtStatusOk);

    return (Subscription*)last_id;
}

void PubSub::unsubscribe(SubscriptionHandle subscription) {
    assert(subscription);

    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    bool result = false;
    auto id = (uint64_t)subscription;
    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->id == id) {
            items.erase(it);
            result = true;
            break;
        }
    }

    tt_check(mutex.release() == TtStatusOk);
    tt_check(result);
}

void PubSub::publish(void* message) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);

    // Iterate over subscribers
    for (auto& it : items) {
        it.callback(message, it.callback_context);
    }

    tt_check(mutex.release() == TtStatusOk);
}

} // namespace
