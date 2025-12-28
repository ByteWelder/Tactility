/**
 * @file RecursiveMutex.h
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
 * Wrapper for FreeRTOS xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex
 * Cannot be used in IRQ mode (within ISR context)
 */
class RecursiveMutex final : public Lock {

private:

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!kernel::isIsr());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle = std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter>(xSemaphoreCreateRecursiveMutex());

public:

    using Lock::lock;

    explicit RecursiveMutex() {
        assert(handle != nullptr);
    }

    ~RecursiveMutex() override = default;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return success result
     */
    bool lock(TickType_t timeout) const override {
        assert(!kernel::isIsr());
        return xSemaphoreTakeRecursive(handle.get(), timeout) == pdPASS;
    }

    /** Attempt to unlock the mutex.
     * @return success result
     */
    bool unlock() const override {
        assert(!kernel::isIsr());
        return xSemaphoreGiveRecursive(handle.get()) == pdPASS;
    }

    /** @return the owner of the thread */
    ThreadId getOwner() const {
        assert(!kernel::isIsr());
        return xSemaphoreGetMutexHolder(handle.get());
    }
};

} // namespace
