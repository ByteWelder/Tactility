#include "Semaphore.h"
#include "Check.h"
#include "CoreDefines.h"

namespace tt {

Semaphore::Semaphore(uint32_t maxCount, uint32_t initialCount) {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert((maxCount > 0U) && (initialCount <= maxCount));

    if (maxCount == 1U) {
        handle = xSemaphoreCreateBinary();
        if ((handle != nullptr) && (initialCount != 0U)) {
            if (xSemaphoreGive(handle) != pdPASS) {
                vSemaphoreDelete(handle);
                handle = nullptr;
            }
        }
    } else {
        handle = xSemaphoreCreateCounting(maxCount, initialCount);
    }

    tt_check(handle);
}

Semaphore::~Semaphore() {
    tt_assert(!TT_IS_IRQ_MODE());
    vSemaphoreDelete(handle);
}

TtStatus Semaphore::acquire(uint32_t timeout) const {
    if (TT_IS_IRQ_MODE()) {
        if (timeout != 0U) {
            return TtStatusErrorParameter;
        } else {
            BaseType_t yield = pdFALSE;

            if (xSemaphoreTakeFromISR(handle, &yield) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
                return TtStatusOk;
            }
        }
    } else {
        if (xSemaphoreTake(handle, (TickType_t)timeout) != pdPASS) {
            if (timeout != 0U) {
                return TtStatusErrorTimeout;
            } else {
                return TtStatusErrorResource;
            }
        } else {
            return TtStatusOk;
        }
    }
}

TtStatus Semaphore::release() const {
    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield = pdFALSE;

        if (xSemaphoreGiveFromISR(handle, &yield) != pdTRUE) {
            return TtStatusErrorResource;
        } else {
            portYIELD_FROM_ISR(yield);
            return TtStatusOk;
        }
    } else {
        if (xSemaphoreGive(handle) != pdPASS) {
            return TtStatusErrorResource;
        } else {
            return TtStatusOk;
        }
    }
}

uint32_t Semaphore::getCount() const {
    if (TT_IS_IRQ_MODE()) {
        // TODO: uxSemaphoreGetCountFromISR is not supported on esp-idf 5.1.2 - perhaps later on?
#ifdef uxSemaphoreGetCountFromISR
        return uxSemaphoreGetCountFromISR(handle);
#else
        return uxQueueMessagesWaitingFromISR((QueueHandle_t)hSemaphore);
#endif
    } else {
        return uxSemaphoreGetCount(handle);
    }
}

} // namespace
