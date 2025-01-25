#include "Semaphore.h"
#include "Check.h"
#include "CoreDefines.h"

namespace tt {

static inline struct QueueDefinition* createHandle(uint32_t maxCount, uint32_t initialCount) {
    assert((maxCount > 0U) && (initialCount <= maxCount));

    if (maxCount == 1U) {
        auto handle = xSemaphoreCreateBinary();
        if ((handle != nullptr) && (initialCount != 0U)) {
            if (xSemaphoreGive(handle) != pdPASS) {
                vSemaphoreDelete(handle);
                handle = nullptr;
            }
        }
        return handle;
    } else {
        return xSemaphoreCreateCounting(maxCount, initialCount);
    }
}

Semaphore::Semaphore(uint32_t maxCount, uint32_t initialCount) : handle(createHandle(maxCount, initialCount)){
    assert(!TT_IS_IRQ_MODE());
    tt_check(handle != nullptr);
}

Semaphore::~Semaphore() {
    assert(!TT_IS_IRQ_MODE());
}

bool Semaphore::acquire(uint32_t timeout) const {
    if (TT_IS_IRQ_MODE()) {
        if (timeout != 0U) {
            return false;
        } else {
            BaseType_t yield = pdFALSE;

            if (xSemaphoreTakeFromISR(handle.get(), &yield) != pdPASS) {
                return false;
            } else {
                portYIELD_FROM_ISR(yield);
                return true;
            }
        }
    } else {
        return xSemaphoreTake(handle.get(), (TickType_t)timeout) == pdPASS;
    }
}

bool Semaphore::release() const {
    if (TT_IS_IRQ_MODE()) {
        BaseType_t yield = pdFALSE;
        if (xSemaphoreGiveFromISR(handle.get(), &yield) != pdTRUE) {
            return false;
        } else {
            portYIELD_FROM_ISR(yield);
            return true;
        }
    } else {
        return xSemaphoreGive(handle.get()) == pdPASS;
    }
}

uint32_t Semaphore::getCount() const {
    if (TT_IS_IRQ_MODE()) {
        // TODO: uxSemaphoreGetCountFromISR is not supported on esp-idf 5.1.2 - perhaps later on?
#ifdef uxSemaphoreGetCountFromISR
        return uxSemaphoreGetCountFromISR(handle.get());
#else
        return uxQueueMessagesWaitingFromISR((QueueHandle_t)hSemaphore);
#endif
    } else {
        return uxSemaphoreGetCount(handle.get());
    }
}

} // namespace
