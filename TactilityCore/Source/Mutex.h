/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "Thread.h"
#include "RtosCompatSemaphore.h"
#include "Check.h"
#include "Lockable.h"
#include <memory>

namespace tt {

/**
 * Wrapper for FreeRTOS xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex
 * Cannot be used in IRQ mode (within ISR context)
 */
class Mutex final : public Lockable {

public:

    enum class Type {
        Normal,
        Recursive,
    };

private:

    struct SemaphoreHandleDeleter {
        void operator()(QueueHandle_t handleToDelete) {
            assert(!TT_IS_IRQ_MODE());
            vSemaphoreDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<QueueHandle_t>, SemaphoreHandleDeleter> handle;
    Type type;

public:

    using Lockable::lock;

    explicit Mutex(Type type = Type::Normal);
    ~Mutex() final = default;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return success result
     */
    bool lock(TickType_t timeout) const final;

    /** Attempt to unlock the mutex.
     * @return success result
     */
    bool unlock() const final;

    /** @return the owner of the thread */
    ThreadId getOwner() const;
};

} // namespace
