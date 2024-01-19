#include "tt_stream_buffer.h"

#include "check.h"
#include "core_defines.h"
#include "core_types.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#else
#include "FreeRTOS.h"
#include "stream_buffer.h"
#endif

StreamBuffer* tt_stream_buffer_alloc(size_t size, size_t trigger_level) {
    tt_assert(size != 0);

    StreamBufferHandle_t handle = xStreamBufferCreate(size, trigger_level);
    tt_check(handle);

    return handle;
};

void tt_stream_buffer_free(StreamBuffer* stream_buffer) {
    tt_assert(stream_buffer);
    vStreamBufferDelete(stream_buffer);
};

bool tt_stream_set_trigger_level(StreamBuffer* stream_buffer, size_t trigger_level) {
    tt_assert(stream_buffer);
    return xStreamBufferSetTriggerLevel(stream_buffer, trigger_level) == pdTRUE;
};

size_t tt_stream_buffer_send(
    StreamBuffer* stream_buffer,
    const void* data,
    size_t length,
    uint32_t timeout
) {
    size_t ret;

    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield;
        ret = xStreamBufferSendFromISR(stream_buffer, data, length, &yield);
        portYIELD_FROM_ISR(yield);
    } else {
        ret = xStreamBufferSend(stream_buffer, data, length, timeout);
    }

    return ret;
};

size_t tt_stream_buffer_receive(
    StreamBuffer* stream_buffer,
    void* data,
    size_t length,
    uint32_t timeout
) {
    size_t ret;

    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield;
        ret = xStreamBufferReceiveFromISR(stream_buffer, data, length, &yield);
        portYIELD_FROM_ISR(yield);
    } else {
        ret = xStreamBufferReceive(stream_buffer, data, length, timeout);
    }

    return ret;
}

size_t tt_stream_buffer_bytes_available(StreamBuffer* stream_buffer) {
    return xStreamBufferBytesAvailable(stream_buffer);
};

size_t tt_stream_buffer_spaces_available(StreamBuffer* stream_buffer) {
    return xStreamBufferSpacesAvailable(stream_buffer);
};

bool tt_stream_buffer_is_full(StreamBuffer* stream_buffer) {
    return xStreamBufferIsFull(stream_buffer) == pdTRUE;
};

bool tt_stream_buffer_is_empty(StreamBuffer* stream_buffer) {
    return (xStreamBufferIsEmpty(stream_buffer) == pdTRUE);
};

TtStatus tt_stream_buffer_reset(StreamBuffer* stream_buffer) {
    if (xStreamBufferReset(stream_buffer) == pdPASS) {
        return TtStatusOk;
    } else {
        return TtStatusError;
    }
}