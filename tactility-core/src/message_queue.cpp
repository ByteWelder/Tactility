#include "message_queue.h"
#include "check.h"
#include "kernel.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#endif

MessageQueue* tt_message_queue_alloc(uint32_t msg_count, uint32_t msg_size) {
    tt_assert((tt_kernel_is_irq() == 0U) && (msg_count > 0U) && (msg_size > 0U));

    QueueHandle_t handle = xQueueCreate(msg_count, msg_size);
    tt_check(handle);

    return ((MessageQueue*)handle);
}

void tt_message_queue_free(MessageQueue* instance) {
    tt_assert(tt_kernel_is_irq() == 0U);
    tt_assert(instance);

    vQueueDelete((QueueHandle_t)instance);
}

TtStatus tt_message_queue_put(MessageQueue* instance, const void* msg_ptr, uint32_t timeout) {
    QueueHandle_t hQueue = (QueueHandle_t)instance;
    TtStatus stat;
    BaseType_t yield;

    stat = TtStatusOk;

    if (tt_kernel_is_irq() != 0U) {
        if ((hQueue == NULL) || (msg_ptr == NULL) || (timeout != 0U)) {
            stat = TtStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if (xQueueSendToBackFromISR(hQueue, msg_ptr, &yield) != pdTRUE) {
                stat = TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((hQueue == NULL) || (msg_ptr == NULL)) {
            stat = TtStatusErrorParameter;
        } else {
            if (xQueueSendToBack(hQueue, msg_ptr, (TickType_t)timeout) != pdPASS) {
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

TtStatus tt_message_queue_get(MessageQueue* instance, void* msg_ptr, uint32_t timeout_ticks) {
    QueueHandle_t hQueue = (QueueHandle_t)instance;
    TtStatus stat;
    BaseType_t yield;

    stat = TtStatusOk;

    if (tt_kernel_is_irq() != 0U) {
        if ((hQueue == NULL) || (msg_ptr == NULL) || (timeout_ticks != 0U)) {
            stat = TtStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if (xQueueReceiveFromISR(hQueue, msg_ptr, &yield) != pdPASS) {
                stat = TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if ((hQueue == NULL) || (msg_ptr == NULL)) {
            stat = TtStatusErrorParameter;
        } else {
            if (xQueueReceive(hQueue, msg_ptr, (TickType_t)timeout_ticks) != pdPASS) {
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

uint32_t tt_message_queue_get_capacity(MessageQueue* instance) {
    StaticQueue_t* mq = (StaticQueue_t*)instance;
    uint32_t capacity;

    if (mq == NULL) {
        capacity = 0U;
    } else {
        /* capacity = pxQueue->uxLength */
        capacity = mq->uxDummy4[1];
    }

    /* Return maximum number of messages */
    return (capacity);
}

uint32_t tt_message_queue_get_message_size(MessageQueue* instance) {
    StaticQueue_t* mq = (StaticQueue_t*)instance;
    uint32_t size;

    if (mq == NULL) {
        size = 0U;
    } else {
        /* size = pxQueue->uxItemSize */
        size = mq->uxDummy4[2];
    }

    /* Return maximum message size */
    return (size);
}

uint32_t tt_message_queue_get_count(MessageQueue* instance) {
    QueueHandle_t hQueue = (QueueHandle_t)instance;
    UBaseType_t count;

    if (hQueue == NULL) {
        count = 0U;
    } else if (tt_kernel_is_irq() != 0U) {
        count = uxQueueMessagesWaitingFromISR(hQueue);
    } else {
        count = uxQueueMessagesWaiting(hQueue);
    }

    /* Return number of queued messages */
    return ((uint32_t)count);
}

uint32_t tt_message_queue_get_space(MessageQueue* instance) {
    StaticQueue_t* mq = (StaticQueue_t*)instance;
    uint32_t space;
    uint32_t isrm;

    if (mq == NULL) {
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

TtStatus tt_message_queue_reset(MessageQueue* instance) {
    QueueHandle_t hQueue = (QueueHandle_t)instance;
    TtStatus stat;

    if (tt_kernel_is_irq() != 0U) {
        stat = TtStatusErrorISR;
    } else if (hQueue == NULL) {
        stat = TtStatusErrorParameter;
    } else {
        stat = TtStatusOk;
        (void)xQueueReset(hQueue);
    }

    /* Return execution status */
    return (stat);
}
