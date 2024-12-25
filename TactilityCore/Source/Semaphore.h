#pragma once

#include "CoreTypes.h"
#include "Thread.h"

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
    SemaphoreHandle_t handle;
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
    bool acquire(uint32_t timeoutTicks) const;

    /** Release semaphore */
    bool release() const;

    /** @return semaphore count */
    uint32_t getCount() const;
};

} // namespace
