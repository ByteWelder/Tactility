#include "semaphore.h"
#include "check.h"
#include "core_defines.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

Semaphore* tt_semaphore_alloc(uint32_t max_count, uint32_t initial_count) {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert((max_count > 0U) && (initial_count <= max_count));

    SemaphoreHandle_t hSemaphore = NULL;
    if (max_count == 1U) {
        hSemaphore = xSemaphoreCreateBinary();
        if ((hSemaphore != NULL) && (initial_count != 0U)) {
            if (xSemaphoreGive(hSemaphore) != pdPASS) {
                vSemaphoreDelete(hSemaphore);
                hSemaphore = NULL;
            }
        }
    } else {
        hSemaphore = xSemaphoreCreateCounting(max_count, initial_count);
    }

    tt_check(hSemaphore);

    return (Semaphore*)hSemaphore;
}

void tt_semaphore_free(Semaphore* instance) {
    tt_assert(instance);
    tt_assert(!TT_IS_IRQ_MODE());

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;

    vSemaphoreDelete(hSemaphore);
}

TtStatus tt_semaphore_acquire(Semaphore* instance, uint32_t timeout) {
    tt_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    TtStatus status;
    BaseType_t yield;

    status = TtStatusOk;

    if (TT_IS_IRQ_MODE()) {
        if (timeout != 0U) {
            status = TtStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if (xSemaphoreTakeFromISR(hSemaphore, &yield) != pdPASS) {
                status = TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }
    } else {
        if (xSemaphoreTake(hSemaphore, (TickType_t)timeout) != pdPASS) {
            if (timeout != 0U) {
                status = TtStatusErrorTimeout;
            } else {
                status = TtStatusErrorResource;
            }
        }
    }

    return status;
}

TtStatus tt_semaphore_release(Semaphore* instance) {
    tt_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    TtStatus stat;
    BaseType_t yield;

    stat = TtStatusOk;

    if (TT_IS_IRQ_MODE()) {
        yield = pdFALSE;

        if (xSemaphoreGiveFromISR(hSemaphore, &yield) != pdTRUE) {
            stat = TtStatusErrorResource;
        } else {
            portYIELD_FROM_ISR(yield);
        }
    } else {
        if (xSemaphoreGive(hSemaphore) != pdPASS) {
            stat = TtStatusErrorResource;
        }
    }

    /* Return execution status */
    return (stat);
}

uint32_t tt_semaphore_get_count(Semaphore* instance) {
    tt_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    uint32_t count;

    if (TT_IS_IRQ_MODE()) {
        // TODO: uxSemaphoreGetCountFromISR is not supported on esp-idf 5.1.2 - perhaps later on?
#ifdef uxSemaphoreGetCountFromISR
        count = (uint32_t)uxSemaphoreGetCountFromISR(hSemaphore);
#else
        count = (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)hSemaphore);
#endif
    } else {
        count = (uint32_t)uxSemaphoreGetCount(hSemaphore);
    }

    /* Return number of tokens */
    return (count);
}
