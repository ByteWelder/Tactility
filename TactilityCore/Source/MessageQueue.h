/**
 * @file MessageQueue.h
 *
 * MessageQueue is a wrapper for FreeRTOS xQueue functionality.
 * There is no additional thread-safety on top of the xQueue functionality,
 * so make sure you create a lock if needed.
 */
#pragma once

#include "CoreTypes.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#endif

namespace tt {

class MessageQueue {
private:
    QueueHandle_t queue_handle;

public:
    /** Allocate message queue
     *
     * @param[in]  msg_count  The message count
     * @param[in]  msg_size   The message size
     */
    MessageQueue(uint32_t msg_count, uint32_t msg_size);

    ~MessageQueue();

    /** Put message into queue
     *
     * @param      instance  pointer to MessageQueue instance
     * @param[in]  msg_ptr   The message pointer
     * @param[in]  timeout   The timeout
     * @param[in]  msg_prio  The message prio
     *
     * @return     The status.
     */
    TtStatus put(const void* msg_ptr, uint32_t timeout);

    /** Get message from queue
     *
     * @param      instance  pointer to MessageQueue instance
     * @param      msg_ptr   The message pointer
     * @param      msg_prio  The message prioority
     * @param[in]  timeout_ticks   The timeout
     *
     * @return     The status.
     */
    TtStatus get(void* msg_ptr, uint32_t timeout_ticks);

    /** Get queue capacity
     *
     * @param      instance  pointer to MessageQueue instance
     *
     * @return     capacity in object count
     */
    uint32_t getCapacity() const;

    /** Get message size
     *
     * @param      instance  pointer to MessageQueue instance
     *
     * @return     Message size in bytes
     */
    uint32_t getMessageSize() const;

    /** Get message count in queue
     *
     * @param      instance  pointer to MessageQueue instance
     *
     * @return     Message count
     */
    uint32_t getCount() const;

    /** Get queue available space
     *
     * @param      instance  pointer to MessageQueue instance
     *
     * @return     Message count
     */
    uint32_t getSpace() const;

    /** Reset queue
     *
     * @param      instance  pointer to MessageQueue instance
     *
     * @return     The status.
     */
    TtStatus reset();
};

} // namespace
