#include "Tactility/StreamBuffer.h"

#include "Tactility/Check.h"
#include "Tactility/kernel/Kernel.h"

namespace tt {

static StreamBufferHandle_t createStreamBuffer(size_t size, size_t triggerLevel) {
    assert(size != 0);
    return xStreamBufferCreate(size, triggerLevel);
}

StreamBuffer::StreamBuffer(size_t size, size_t triggerLevel) : handle(createStreamBuffer(size, triggerLevel)) {
    tt_check(handle);
};

bool StreamBuffer::setTriggerLevel(size_t triggerLevel) const {
    return xStreamBufferSetTriggerLevel(handle.get(), triggerLevel) == pdTRUE;
};

size_t StreamBuffer::send(
    const void* data,
    size_t length,
    uint32_t timeout
) const {
    if (kernel::isIsr()) {
        BaseType_t yield;
        size_t result = xStreamBufferSendFromISR(handle.get(), data, length, &yield);
        portYIELD_FROM_ISR(yield);
        return result;
    } else {
        return xStreamBufferSend(handle.get(), data, length, timeout);
    }
};

size_t StreamBuffer::receive(
    void* data,
    size_t length,
    uint32_t timeout
) const {
    if (kernel::isIsr()) {
        BaseType_t yield;
        size_t result = xStreamBufferReceiveFromISR(handle.get(), data, length, &yield);
        portYIELD_FROM_ISR(yield);
        return result;
    } else {
        return xStreamBufferReceive(handle.get(), data, length, timeout);
    }
}

size_t StreamBuffer::getAvailableReadBytes() const {
    return xStreamBufferBytesAvailable(handle.get());
};

size_t StreamBuffer::getAvailableWriteBytes() const {
    return xStreamBufferSpacesAvailable(handle.get());
};

bool StreamBuffer::isFull() const {
    return xStreamBufferIsFull(handle.get()) == pdTRUE;
};

bool StreamBuffer::isEmpty() const {
    return xStreamBufferIsEmpty(handle.get()) == pdTRUE;
};

bool StreamBuffer::reset() const {
    return xStreamBufferReset(handle.get()) == pdPASS;
}

} // namespace
