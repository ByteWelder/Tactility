#pragma once

#include <memory>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#else
#include "FreeRTOS.h"
#include "stream_buffer.h"
#endif

namespace tt {

/**
 * Stream buffers are used to send a continuous stream of data from one task or
 * interrupt to another. Their implementation is light weight, making them
 * particularly suited for interrupt to task and core to core communication
 * scenarios.
 *
 * **NOTE**: Stream buffer implementation assumes there is only one task or
 * interrupt that will write to the buffer (the writer), and only one task or
 * interrupt that will read from the buffer (the reader).
 */
class StreamBuffer final {

    struct StreamBufferHandleDeleter {
        void operator()(StreamBufferHandle_t handleToDelete) {
            vStreamBufferDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<StreamBufferHandle_t>, StreamBufferHandleDeleter> handle;

public:

    /**
     * Stream buffer implementation assumes there is only one task or
     * interrupt that will write to the buffer (the writer), and only one task or
     * interrupt that will read from the buffer (the reader).
     *
     * @param[in] size The total number of bytes the stream buffer will be able to hold at any one time.
     * @param[in] triggerLevel The number of bytes that must be in the stream buffer
     * before a task that is blocked on the stream buffer to wait for data is moved out of the blocked state.
     * @return The stream buffer instance.
     */
    StreamBuffer(size_t size, size_t triggerLevel);

    ~StreamBuffer() = default;

    /**
     * @brief Set trigger level for stream buffer.
     * A stream buffer's trigger level is the number of bytes that must be in the
     * stream buffer before a task that is blocked on the stream buffer to
     * wait for data is moved out of the blocked state.
     *
     * @param[in] triggerLevel The new trigger level for the stream buffer.
     * @return true if trigger level can be be updated (new trigger level was less than or equal to the stream buffer's length).
     * @return false if trigger level can't be be updated (new trigger level was greater than the stream buffer's length).
     */
    bool setTriggerLevel(size_t triggerLevel) const;

    /**
     * @brief Sends bytes to a stream buffer. The bytes are copied into the stream buffer.
     * Wakes up task waiting for data to become available if called from ISR.
     *
     * @param[in] data A pointer to the data that is to be copied into the stream buffer.
     * @param[in] length The maximum number of bytes to copy from data into the stream buffer.
     * @param[in] timeout The maximum amount of time the task should remain in the
     * Blocked state to wait for space to become available if the stream buffer is full.
     * Will return immediately if timeout is zero.
     * Setting timeout to portMAX_DELAY will cause the task to wait indefinitely.
     * Ignored if called from ISR.
     * @return The number of bytes actually written to the stream buffer.
     */
    size_t send(
        const void* data,
        size_t length,
        uint32_t timeout
    ) const;

    /**
     * @brief Receives bytes from a stream buffer.
     * Wakes up task waiting for space to become available if called from ISR.
     *
     * @param[in] data A pointer to the buffer into which the received bytes will be
     * copied.
     * @param[in] length The length of the buffer pointed to by the data parameter.
     * @param[in] timeout The maximum amount of time the task should remain in the
     * Blocked state to wait for data to become available if the stream buffer is empty.
     * Will return immediately if timeout is zero.
     * Setting timeout to portMAX_DELAY will cause the task to wait indefinitely.
     * Ignored if called from ISR.
     * @return The number of bytes read from the stream buffer, if any.
     */
    size_t receive(
        void* data,
        size_t length,
        uint32_t timeout
    ) const;

    /**
     * @brief Queries a stream buffer to see how much data it contains, which is equal to
     * the number of bytes that can be read from the stream buffer before the stream
     * buffer would be empty.
     *
     * @return The number of bytes that can be read from the stream buffer before
     * the stream buffer would be empty.
     */
    size_t getAvailableReadBytes() const;

    /**
     * @brief Queries a stream buffer to see how much free space it contains, which is
     * equal to the amount of data that can be sent to the stream buffer before it
     * is full.
     *
     * @return The number of bytes that can be written to the stream buffer before
     * the stream buffer would be full.
     */
    size_t getAvailableWriteBytes() const;

    /**
     * @brief Queries a stream buffer to see if it is full.
     *
     * @return true if the stream buffer is full.
     * @return false if the stream buffer is not full.
     */
    bool isFull() const;

    /**
     * @brief Queries a stream buffer to see if it is empty.
     *
     * @param stream_buffer The stream buffer instance.
     * @return true if the stream buffer is empty.
     * @return false if the stream buffer is not empty.
     */
    bool isEmpty() const;

    /**
     * @brief Resets a stream buffer to its initial, empty, state. Any data that was
     * in the stream buffer is discarded. A stream buffer can only be reset if there
     * are no tasks blocked waiting to either send to or receive from the stream buffer.
     *
     * @return TtStatusOk if the stream buffer is reset.
     * @return TtStatusError if there was a task blocked waiting to send to or read
     * from the stream buffer then the stream buffer is not reset.
     */
    bool reset() const;
};


} // namespace
