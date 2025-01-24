#include "MessageQueue.h"
#include "Check.h"
#include "kernel/Kernel.h"

namespace tt {

MessageQueue::MessageQueue(uint32_t capacity, uint32_t msg_size) {
    assert(!TT_IS_ISR() && (capacity > 0U) && (msg_size > 0U));
    queue_handle = xQueueCreate(capacity, msg_size);
    tt_check(queue_handle);
}

MessageQueue::~MessageQueue() {
    assert(!TT_IS_ISR());
    vQueueDelete(queue_handle);
}

bool MessageQueue::put(const void* message, uint32_t timeout) {
    bool result = true;
    BaseType_t yield;

    if (TT_IS_ISR()) {
        if ((queue_handle == nullptr) || (message == nullptr) || (timeout != 0U)) {
            result = false;
        } else {
            yield = pdFALSE;

            if (xQueueSendToBackFromISR(queue_handle, message, &yield) != pdTRUE) {
                result = false;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else if ((queue_handle == nullptr) || (message == nullptr)) {
        result = false;
    } else if (xQueueSendToBack(queue_handle, message, (TickType_t)timeout) != pdPASS) {
        result = false;
    }

    return result;
}

bool MessageQueue::get(void* msg_ptr, uint32_t timeout_ticks) {
    bool result = true;
    BaseType_t yield;


    if (TT_IS_ISR()) {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr) || (timeout_ticks != 0U)) {
            result = false;
        } else {
            yield = pdFALSE;

            if (xQueueReceiveFromISR(queue_handle, msg_ptr, &yield) != pdPASS) {
                result = false;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr)) {
            result = false;
        } else if (xQueueReceive(queue_handle, msg_ptr, (TickType_t)timeout_ticks) != pdPASS) {
            result = false;
        }
    }

    return result;
}

uint32_t MessageQueue::getCapacity() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
    if (mq == nullptr) {
        return 0U;
    } else {
        return mq->uxDummy4[1];
    }
}

uint32_t MessageQueue::getMessageSize() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
    if (mq == nullptr) {
        return 0U;
    } else {
        return mq->uxDummy4[2];
    }
}

uint32_t MessageQueue::getCount() const {
    UBaseType_t count;

    if (queue_handle == nullptr) {
        count = 0U;
    } else if (TT_IS_ISR()) {
        count = uxQueueMessagesWaitingFromISR(queue_handle);
    } else {
        count = uxQueueMessagesWaiting(queue_handle);
    }

    /* Return number of queued messages */
    return (uint32_t)count;
}

uint32_t MessageQueue::getSpace() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
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
    if (queue_handle == nullptr) {
        return false;
    } else {
        xQueueReset(queue_handle);
        return true;
    }
}

} // namespace
