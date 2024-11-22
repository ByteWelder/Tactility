/**
 * @file pubsub.h
 * PubSub
 */
#pragma once

namespace tt {

/** PubSub Callback type */
typedef void (*PubSubCallback)(const void* message, void* context);

/** PubSub type */
typedef struct PubSub PubSub;

/** PubSubSubscription type */
typedef struct PubSubSubscription PubSubSubscription;

/** Allocate PubSub
 *
 * Reentrable, Not threadsafe, one owner
 *
 * @return     pointer to PubSub instance
 */
PubSub* tt_pubsub_alloc();

/** Free PubSub
 * 
 * @param      pubsub  PubSub instance
 */
void tt_pubsub_free(PubSub* pubsub);

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
tt_pubsub_subscribe(PubSub* pubsub, PubSubCallback callback, void* callback_context);

/** Unsubscribe from PubSub
 * 
 * No use of `pubsub_subscription` allowed after call of this method
 * Threadsafe, Reentrable.
 *
 * @param      pubsub               pointer to PubSub instance
 * @param      pubsub_subscription  pointer to PubSubSubscription instance
 */
void tt_pubsub_unsubscribe(PubSub* pubsub, PubSubSubscription* pubsub_subscription);

/** Publish message to PubSub
 *
 * Threadsafe, Reentrable.
 * 
 * @param      pubsub   pointer to PubSub instance
 * @param      message  message pointer to publish
 */
void tt_pubsub_publish(PubSub* pubsub, void* message);

} // namespace
