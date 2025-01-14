#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** A handle that represents a message queue instance */
typedef void* MessageQueueHandle;

/**
 * Allocate a new message queue in memory.
 * @param[in] capacity how many messages this queue can contain before it starts blocking input
 * @param[in] messageSize the size of a message
 */
MessageQueueHandle tt_message_queue_alloc(uint32_t capacity, uint32_t messageSize);

/** Free up the memory of a queue (dealloc) */
void tt_message_queue_free(MessageQueueHandle handle);

/**
 * Put (post) a message in the queue
 * @param[in] handle the queue handle
 * @param[in] message the message of the correct size - its data will be copied
 * @param[timeout] timeout the amount of ticks to wait until the message is queued
 * @return true if the item was successfully queued
 */
bool tt_message_queue_put(MessageQueueHandle handle, const void* message, TickType_t timeout);

/**
 * Get the oldest message from the queue.
 * @param[in] handle the queue handle
 * @param[out] message a pointer to a message of the correct size
 * @param[in] timeout the amount of ticks to wait until a message was copied
 * @return true if a message was successfully copied
 */
bool tt_message_queue_get(MessageQueueHandle handle, void* message, TickType_t timeout);

/** @return the total amount of messages that this queue can hold */
uint32_t tt_message_queue_get_capacity(MessageQueueHandle handle);

/** @return the size of a single message in the queue */
uint32_t tt_message_queue_get_message_size(MessageQueueHandle handle);

/** @return the current amount of items in the queue */
uint32_t tt_message_queue_get_count(MessageQueueHandle handle);

/** @return the remaining capacity in the queue */
uint32_t tt_message_queue_get_space(MessageQueueHandle handle);

/**
 * Remove all items from the queue (if any)
 * @return true on failure
 */
bool tt_message_queue_reset(MessageQueueHandle handle);

#ifdef __cplusplus
}
#endif