#include "pubsub.h"
#include "check.h"
#include "mutex.h"

#include <m-list.h>

struct PubSubSubscription {
    PubSubCallback callback;
    void* callback_context;
};

LIST_DEF(PubSubSubscriptionList, PubSubSubscription, M_POD_OPLIST);

struct PubSub {
    PubSubSubscriptionList_t items;
    Mutex mutex;
};

PubSub* tt_pubsub_alloc() {
    PubSub* pubsub = malloc(sizeof(PubSub));

    pubsub->mutex = tt_mutex_alloc(MutexTypeNormal);
    tt_assert(pubsub->mutex);

    PubSubSubscriptionList_init(pubsub->items);

    return pubsub;
}

void tt_pubsub_free(PubSub* pubsub) {
    tt_assert(pubsub);

    tt_check(PubSubSubscriptionList_size(pubsub->items) == 0);

    PubSubSubscriptionList_clear(pubsub->items);

    tt_mutex_free(pubsub->mutex);

    free(pubsub);
}

PubSubSubscription* tt_pubsub_subscribe(PubSub* pubsub, PubSubCallback callback, void* callback_context) {
    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);
    // put uninitialized item to the list
    PubSubSubscription* item = PubSubSubscriptionList_push_raw(pubsub->items);

    // initialize item
    item->callback = callback;
    item->callback_context = callback_context;

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);

    return item;
}

void tt_pubsub_unsubscribe(PubSub* pubsub, PubSubSubscription* pubsub_subscription) {
    tt_assert(pubsub);
    tt_assert(pubsub_subscription);

    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);
    bool result = false;

    // iterate over items
    PubSubSubscriptionList_it_t it;
    for (PubSubSubscriptionList_it(it, pubsub->items); !PubSubSubscriptionList_end_p(it);
         PubSubSubscriptionList_next(it)) {
        const PubSubSubscription* item = PubSubSubscriptionList_cref(it);

        // if the iterator is equal to our element
        if (item == pubsub_subscription) {
            PubSubSubscriptionList_remove(pubsub->items, it);
            result = true;
            break;
        }
    }

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);
    tt_check(result);
}

void tt_pubsub_publish(PubSub* pubsub, void* message) {
    tt_check(tt_mutex_acquire(pubsub->mutex, TtWaitForever) == TtStatusOk);

    // iterate over subscribers
    PubSubSubscriptionList_it_t it;
    for (PubSubSubscriptionList_it(it, pubsub->items); !PubSubSubscriptionList_end_p(it);
         PubSubSubscriptionList_next(it)) {
        const PubSubSubscription* item = PubSubSubscriptionList_cref(it);
        item->callback(message, item->callback_context);
    }

    tt_check(tt_mutex_release(pubsub->mutex) == TtStatusOk);
}
