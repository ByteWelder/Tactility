/**
 * @file Mutex.h
 * Mutex
 */
#pragma once

#include "Lock.h"
#include "RtosCompatSemaphore.h"
#include "Thread.h"
#include "kernel/Kernel.h"

#include <memory>
#include <cassert>

namespace tt {

/**
 * Wrapper for FreeRTOS xSemaphoreCreateMutex
 * Cannot be used in IRQ mode (within ISR context)
 */
class Mutex final : public Lock {

private:

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!kernel::isIsr());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle = std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter>(xSemaphoreCreateMutex());

public:

    using Lock::lock;

    explicit Mutex() {
        assert(handle != nullptr);
    }

    ~Mutex() override = default;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return success result
     */
    bool lock(TickType_t timeout) const override {
        assert(!kernel::isIsr());
        return xSemaphoreTake(handle.get(), timeout) == pdPASS;
    }

    /** Attempt to unlock the mutex.
     * @return success result
     */
    bool unlock() const override {
        assert(!kernel::isIsr());
        return xSemaphoreGive(handle.get()) == pdPASS;
    }

    /** @return the owner of the thread */
    ThreadId getOwner() const {
        assert(!kernel::isIsr());
        return xSemaphoreGetMutexHolder(handle.get());
    }
};

} // namespace tt
