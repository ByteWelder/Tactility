/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "Check.h"
#include "Lock.h"
#include "RtosCompatSemaphore.h"
#include "Thread.h"
#include "kernel/Kernel.h"
#include <memory>

namespace tt {

/**
 * Wrapper for FreeRTOS xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex
 * Cannot be used in IRQ mode (within ISR context)
 */
class Mutex final : public Lock {

public:
    /**
     * A "Normal" mutex can only be locked once. Even from within the same task/thread.
     * A "Recursive" mutex can be locked again from the same task/thread.
     */
    enum class Type {
        Normal,
        Recursive,
    };

private:

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!kernel::isIsr());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle;
    Type type;

public:

    using Lock::lock;

    explicit Mutex(Type type = Type::Normal);
    ~Mutex() override = default;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return success result
     */
    bool lock(TickType_t timeout) const override;

    /** Attempt to unlock the mutex.
     * @return success result
     */
    bool unlock() const override;

    /** @return the owner of the thread */
    ThreadId getOwner() const;
};

} // namespace
