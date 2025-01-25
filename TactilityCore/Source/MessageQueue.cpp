#include "MessageQueue.h"
#include "Check.h"
#include "kernel/Kernel.h"

namespace tt {

static inline QueueHandle_t createQueue(uint32_t capacity, uint32_t messageSize) {
    assert(!TT_IS_ISR() && (capacity > 0U) && (messageSize > 0U));
    return xQueueCreate(capacity, messageSize);
}

MessageQueue::MessageQueue(uint32_t capacity, uint32_t messageSize) : handle(createQueue(capacity, messageSize)) {
    tt_check(handle != nullptr);
}

MessageQueue::~MessageQueue() {
    assert(!TT_IS_ISR());
}

bool MessageQueue::put(const void* message, TickType_t timeout) {
    bool result = true;
    BaseType_t yield;

    if (TT_IS_ISR()) {
        if ((handle == nullptr) || (message == nullptr) || (timeout != 0U)) {
            result = false;
        } else {
            yield = pdFALSE;

            if (xQueueSendToBackFromISR(handle.get(), message, &yield) != pdTRUE) {
                result = false;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else if ((handle == nullptr) || (message == nullptr)) {
        result = false;
    } else if (xQueueSendToBack(handle.get(), message, (TickType_t)timeout) != pdPASS) {
        result = false;
    }

    return result;
}

bool MessageQueue::get(void* msg_ptr, TickType_t timeout) {
    bool result = true;
    BaseType_t yield;

    if (TT_IS_ISR()) {
        if ((handle == nullptr) || (msg_ptr == nullptr) || (timeout != 0U)) {
            result = false;
        } else {
            yield = pdFALSE;

            if (xQueueReceiveFromISR(handle.get(), msg_ptr, &yield) != pdPASS) {
                result = false;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((handle == nullptr) || (msg_ptr == nullptr)) {
            result = false;
        } else if (xQueueReceive(handle.get(), msg_ptr, (TickType_t)timeout) != pdPASS) {
            result = false;
        }
    }

    return result;
}

uint32_t MessageQueue::getCapacity() const {
    auto* mq = (StaticQueue_t*)(handle.get());
    if (mq == nullptr) {
        return 0U;
    } else {
        return mq->uxDummy4[1];
    }
}

uint32_t MessageQueue::getMessageSize() const {
    auto* mq = (StaticQueue_t*)(handle.get());
    if (mq == nullptr) {
        return 0U;
    } else {
        return mq->uxDummy4[2];
    }
}

uint32_t MessageQueue::getCount() const {
    UBaseType_t count;

    if (handle == nullptr) {
        count = 0U;
    } else if (TT_IS_ISR()) {
        count = uxQueueMessagesWaitingFromISR(handle.get());
    } else {
        count = uxQueueMessagesWaiting(handle.get());
    }

    /* Return number of queued messages */
    return (uint32_t)count;
}

uint32_t MessageQueue::getSpace() const {
    auto* mq = (StaticQueue_t*)(handle.get());
    uint32_t space;
    uint32_t isrm;

    if (mq == nullptr) {
        space = 0U;
    } else if (TT_IS_ISR()) {
        isrm = taskENTER_CRITICAL_FROM_ISR();

        /* space = pxQueue->uxLength - pxQueue->uxMessagesWaiting; */
        space = mq->uxDummy4[1] - mq->uxDummy4[0];

        taskEXIT_CRITICAL_FROM_ISR(isrm);
    } else {
        space = (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)mq);
    }

    return space;
}

bool MessageQueue::reset() {
    tt_check(!TT_IS_ISR());
    if (handle == nullptr) {
        return false;
    } else {
        xQueueReset(handle.get());
        return true;
    }
}

} // namespace
