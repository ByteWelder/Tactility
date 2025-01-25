#include "PubSub.h"
#include "Check.h"

namespace tt {

PubSub::SubscriptionHandle PubSub::subscribe(PubSubCallback callback, void* callbackParameter) {
    mutex.lock();
    items.push_back({
        .id = (++lastId),
        .callback = callback,
        .callbackParameter = callbackParameter});

    mutex.unlock();

    return (Subscription*)lastId;
}

void PubSub::unsubscribe(SubscriptionHandle subscription) {
    assert(subscription);

    mutex.lock();
    bool result = false;
    auto id = (uint64_t)subscription;
    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->id == id) {
            items.erase(it);
            result = true;
            break;
        }
    }

    mutex.unlock();
    tt_check(result);
}

void PubSub::publish(void* message) {
    mutex.lock();

    // Iterate over subscribers
    for (auto& it : items) {
        it.callback(message, it.callbackParameter);
    }

    mutex.unlock();
}

} // namespace
