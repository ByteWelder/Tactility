#pragma once

#include "Thread.h"
#include <cassert>
#include <memory>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

namespace tt {

/**
 * Wrapper for xSemaphoreCreateBinary (max count == 1) and xSemaphoreCreateCounting (max count > 1)
 * Can be used from IRQ/ISR mode, but cannot be created/destroyed from such a context.
 */
class Semaphore {

private:

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!TT_IS_IRQ_MODE());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle;

public:
    /**
     * Cannot be called from IRQ/ISR mode.
     * @param[in] maxCount The maximum count
     * @param[in] initialCount The initial count
     */
    Semaphore(uint32_t maxCount, uint32_t initialCount);

    /** Cannot be called from IRQ/ISR mode. */
    ~Semaphore();

    /** Acquire semaphore */
    bool acquire(uint32_t timeout) const;

    /** Release semaphore */
    bool release() const;

    /** @return semaphore count */
    uint32_t getCount() const;
};

} // namespace
