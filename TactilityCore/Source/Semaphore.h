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
 * Can be used in IRQ mode (within ISR context)
 */
class Semaphore {
private:
    SemaphoreHandle_t handle;
public:
    /**
     * @param[in] maxCount The maximum count
     * @param[in] initialCount The initial count
     */
    Semaphore(uint32_t maxCount, uint32_t initialCount);

    /**
     * @param instance The pointer to Semaphore instance
     */
    ~Semaphore();

    /** Acquire semaphore
     * @param[in] timeout The timeout
     * @return the status
     */
    TtStatus acquire(uint32_t timeout) const;

    /** Release semaphore
     * @return the status
     */
    TtStatus release() const;

    /** Get semaphore count
     * @return semaphore count
     */
    uint32_t getCount() const;
};

} // namespace
