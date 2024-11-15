/**
 * @file message_queue.h
 *
 * MessageQueue is a wrapper for FreeRTOS xQueue functionality.
 * There is no additional thread-safety on top of the xQueue functionality,
 * so make sure you create a lock if needed.
 */
#pragma once

#include "core_types.h"

typedef void MessageQueue;

/** Allocate message queue
 *
 * @param[in]  msg_count  The message count
 * @param[in]  msg_size   The message size
 *
 * @return     pointer to MessageQueue instance
 */
MessageQueue* tt_message_queue_alloc(uint32_t msg_count, uint32_t msg_size);

/** Free queue
 *
 * @param      instance  pointer to MessageQueue instance
 */
void tt_message_queue_free(MessageQueue* instance);

/** Put message into queue
 *
 * @param      instance  pointer to MessageQueue instance
 * @param[in]  msg_ptr   The message pointer
 * @param[in]  timeout   The timeout
 * @param[in]  msg_prio  The message prio
 *
 * @return     The status.
 */
TtStatus tt_message_queue_put(MessageQueue* instance, const void* msg_ptr, uint32_t timeout);

/** Get message from queue
 *
 * @param      instance  pointer to MessageQueue instance
 * @param      msg_ptr   The message pointer
 * @param      msg_prio  The message prioority
 * @param[in]  timeout_ticks   The timeout
 *
 * @return     The status.
 */
TtStatus tt_message_queue_get(MessageQueue* instance, void* msg_ptr, uint32_t timeout_ticks);

/** Get queue capacity
 *
 * @param      instance  pointer to MessageQueue instance
 *
 * @return     capacity in object count
 */
uint32_t tt_message_queue_get_capacity(MessageQueue* instance);

/** Get message size
 *
 * @param      instance  pointer to MessageQueue instance
 *
 * @return     Message size in bytes
 */
uint32_t tt_message_queue_get_message_size(MessageQueue* instance);

/** Get message count in queue
 *
 * @param      instance  pointer to MessageQueue instance
 *
 * @return     Message count
 */
uint32_t tt_message_queue_get_count(MessageQueue* instance);

/** Get queue available space
 *
 * @param      instance  pointer to MessageQueue instance
 *
 * @return     Message count
 */
uint32_t tt_message_queue_get_space(MessageQueue* instance);

/** Reset queue
 *
 * @param      instance  pointer to MessageQueue instance
 *
 * @return     The status.
 */
TtStatus tt_message_queue_reset(MessageQueue* instance);
