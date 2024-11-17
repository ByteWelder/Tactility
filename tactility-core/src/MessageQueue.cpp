#include "MessageQueue.h"
#include "check.h"
#include "kernel.h"

MessageQueue::MessageQueue(uint32_t msg_count, uint32_t msg_size) {
    tt_assert((tt_kernel_is_irq() == 0U) && (msg_count > 0U) && (msg_size > 0U));
    queue_handle = xQueueCreate(msg_count, msg_size);
    tt_check(queue_handle);
}

MessageQueue::~MessageQueue() {
    tt_assert(tt_kernel_is_irq() == 0U);
    vQueueDelete(queue_handle);
}

TtStatus MessageQueue::put(const void* msg_ptr, uint32_t timeout) {
    TtStatus stat;
    BaseType_t yield;

    stat = TtStatusOk;

    if (tt_kernel_is_irq() != 0U) {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr) || (timeout != 0U)) {
            stat = TtStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if (xQueueSendToBackFromISR(queue_handle, msg_ptr, &yield) != pdTRUE) {
                stat = TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr)) {
            stat = TtStatusErrorParameter;
        } else {
            if (xQueueSendToBack(queue_handle, msg_ptr, (TickType_t)timeout) != pdPASS) {
                if (timeout != 0U) {
                    stat = TtStatusErrorTimeout;
                } else {
                    stat = TtStatusErrorResource;
                }
            }
        }
    }

    /* Return execution status */
    return (stat);
}

TtStatus MessageQueue::get(void* msg_ptr, uint32_t timeout_ticks) {
    TtStatus stat;
    BaseType_t yield;

    stat = TtStatusOk;

    if (tt_kernel_is_irq() != 0U) {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr) || (timeout_ticks != 0U)) {
            stat = TtStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if (xQueueReceiveFromISR(queue_handle, msg_ptr, &yield) != pdPASS) {
                stat = TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((queue_handle == nullptr) || (msg_ptr == nullptr)) {
            stat = TtStatusErrorParameter;
        } else {
            if (xQueueReceive(queue_handle, msg_ptr, (TickType_t)timeout_ticks) != pdPASS) {
                if (timeout_ticks != 0U) {
                    stat = TtStatusErrorTimeout;
                } else {
                    stat = TtStatusErrorResource;
                }
            }
        }
    }

    /* Return execution status */
    return (stat);
}

uint32_t MessageQueue::getCapacity() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
    uint32_t capacity;

    if (mq == nullptr) {
        capacity = 0U;
    } else {
        /* capacity = pxQueue->uxLength */
        capacity = mq->uxDummy4[1];
    }

    /* Return maximum number of messages */
    return (capacity);
}

uint32_t MessageQueue::getMessageSize() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
    uint32_t size;

    if (mq == nullptr) {
        size = 0U;
    } else {
        /* size = pxQueue->uxItemSize */
        size = mq->uxDummy4[2];
    }

    /* Return maximum message size */
    return (size);
}

uint32_t MessageQueue::getCount() const {
    UBaseType_t count;

    if (queue_handle == nullptr) {
        count = 0U;
    } else if (tt_kernel_is_irq() != 0U) {
        count = uxQueueMessagesWaitingFromISR(queue_handle);
    } else {
        count = uxQueueMessagesWaiting(queue_handle);
    }

    /* Return number of queued messages */
    return ((uint32_t)count);
}

uint32_t MessageQueue::getSpace() const {
    auto* mq = (StaticQueue_t*)(queue_handle);
    uint32_t space;
    uint32_t isrm;

    if (mq == nullptr) {
        space = 0U;
    } else if (tt_kernel_is_irq() != 0U) {
        isrm = taskENTER_CRITICAL_FROM_ISR();

        /* space = pxQueue->uxLength - pxQueue->uxMessagesWaiting; */
        space = mq->uxDummy4[1] - mq->uxDummy4[0];

        taskEXIT_CRITICAL_FROM_ISR(isrm);
    } else {
        space = (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)mq);
    }

    /* Return number of available slots */
    return (space);
}

TtStatus MessageQueue::reset() {
    TtStatus stat;

    if (tt_kernel_is_irq() != 0U) {
        stat = TtStatusErrorISR;
    } else if (queue_handle == nullptr) {
        stat = TtStatusErrorParameter;
    } else {
        stat = TtStatusOk;
        (void)xQueueReset(queue_handle);
    }

    /* Return execution status */
    return (stat);
}
