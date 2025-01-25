#include "StreamBuffer.h"

#include "Check.h"
#include "CoreDefines.h"
#include "CoreTypes.h"

namespace tt {

StreamBuffer::StreamBuffer(size_t size, size_t triggerLevel) {
    assert(size != 0);
    handle = xStreamBufferCreate(size, triggerLevel);
    tt_check(handle);
};

StreamBuffer::~StreamBuffer() {
    vStreamBufferDelete(handle);
};

bool StreamBuffer::setTriggerLevel(size_t triggerLevel) const {
    return xStreamBufferSetTriggerLevel(handle, triggerLevel) == pdTRUE;
};

size_t StreamBuffer::send(
    const void* data,
    size_t length,
    uint32_t timeout
) const {
    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield;
        size_t result = xStreamBufferSendFromISR(handle, data, length, &yield);
        portYIELD_FROM_ISR(yield);
        return result;
    } else {
        return xStreamBufferSend(handle, data, length, timeout);
    }
};

size_t StreamBuffer::receive(
    void* data,
    size_t length,
    uint32_t timeout
) const {
    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield;
        size_t result = xStreamBufferReceiveFromISR(handle, data, length, &yield);
        portYIELD_FROM_ISR(yield);
        return result;
    } else {
        return xStreamBufferReceive(handle, data, length, timeout);
    }
}

size_t StreamBuffer::getAvailableReadBytes() const {
    return xStreamBufferBytesAvailable(handle);
};

size_t StreamBuffer::getAvailableWriteBytes() const {
    return xStreamBufferSpacesAvailable(handle);
};

bool StreamBuffer::isFull() const {
    return xStreamBufferIsFull(handle) == pdTRUE;
};

bool StreamBuffer::isEmpty() const {
    return xStreamBufferIsEmpty(handle) == pdTRUE;
};

bool StreamBuffer::reset() const {
    return xStreamBufferReset(handle) == pdPASS;
}

} // namespace
