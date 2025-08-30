/**
 * @file MessageQueue.h
 *
 * MessageQueue is a wrapper for FreeRTOS xQueue functionality.
 * There is no additional thread-safety on top of the xQueue functionality,
 * so make sure you create a lock if needed.
 */
#pragma once

#include <memory>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#endif

namespace tt {

/**
 * Message Queue implementation.
 * Calls can be done from ISR/IRQ mode unless otherwise specified.
 */
class MessageQueue {

    struct QueueHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            vQueueDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, QueueHandleDeleter> handle;

public:
    /** Allocate message queue
     * @param[in] capacity Maximum messages in queue
     * @param[in] messageSize The size in bytes of a single message
     */
    MessageQueue(uint32_t capacity, uint32_t messageSize);

    ~MessageQueue();

    /** Post a message to the queue.
     * The message is queued by copy, not by reference.
     * @param[in] message A pointer to a message. The message will be copied into a buffer.
     * @param[in] timeout
     * @return success result
     */
    bool put(const void* message, TickType_t timeout);

    /** Get message from queue
     * @param[out] message A pointer to an already allocated message object
     * @param[in] timeout
     * @return success result
     */
    bool get(void* message, TickType_t timeout);

    /**
     * @return The maximum amount of messages that can be in the queue at any given time.
     */
    uint32_t getCapacity() const;

    /**
     * @return The size of a single message in bytes
     */
    uint32_t getMessageSize() const;

    /**
     * @return How many messages are currently in the queue.
     */
    uint32_t getCount() const;

    /**
     * @return How many messages can be added to the queue before the put() method starts blocking.
     */
    uint32_t getSpace() const;

    /** Reset queue (cannot be called in ISR/IRQ mode)
     * @return success result
     */
    bool reset();
};

} // namespace
