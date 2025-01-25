#include "PubSub.h"
#include "Check.h"

namespace tt {

PubSub::SubscriptionHandle PubSub::subscribe(PubSubCallback callback, void* callbackParameter) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    items.push_back({
        .id = (++lastId),
        .callback = callback,
        .callbackParameter = callbackParameter});

    tt_check(mutex.release() == TtStatusOk);

    return (Subscription*)lastId;
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
        it.callback(message, it.callbackParameter);
    }

    tt_check(mutex.release() == TtStatusOk);
}

} // namespace
