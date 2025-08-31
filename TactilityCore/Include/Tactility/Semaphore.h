#pragma once

#include "Lock.h"
#include "kernel/Kernel.h"
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
class Semaphore final : public Lock {

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!kernel::isIsr());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle;

public:

    using Lock::lock;

    /**
     * Cannot be called from IRQ/ISR mode.
     * @param[in] maxAvailable The maximum count
     * @param[in] initialAvailable The initial count
     */
    Semaphore(uint32_t maxAvailable, uint32_t initialAvailable);

    /**
     * Cannot be called from IRQ/ISR mode.
     * @param[in] maxAvailable The maximum count
     */
    explicit Semaphore(uint32_t maxAvailable) : Semaphore(maxAvailable, maxAvailable) {};

    /** Cannot be called from IRQ/ISR mode. */
    ~Semaphore() override;

    Semaphore(Semaphore& other) : handle(std::move(other.handle)) {}

    /** Acquire semaphore */
    bool acquire(TickType_t timeout) const;

    /** Release semaphore */
    bool release() const;

    bool lock(TickType_t timeout) const override { return acquire(timeout); }

    bool unlock() const override { return release(); }

    /** @return return the amount of times this semaphore can be acquired/locked */
    uint32_t getAvailable() const;
};

} // namespace
