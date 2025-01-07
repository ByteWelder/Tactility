/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "CoreTypes.h"
#include "Thread.h"
#include "RtosCompatSemaphore.h"
#include "Check.h"
#include "Lockable.h"
#include <memory>

namespace tt {

/**
 * Wrapper for FreeRTOS xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex
 * Can be used in IRQ mode (within ISR context)
 */
class Mutex : public Lockable {

public:

    enum Type {
        TypeNormal,
        TypeRecursive,
    };

private:

    SemaphoreHandle_t semaphore;
    Type type;

public:

    explicit Mutex(Type type = TypeNormal);
    ~Mutex() override;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return status result
     */
    TtStatus acquire(TickType_t timeout) const;

    /** Attempt to unlock the mutex.
     * @return status result
     */
    TtStatus release() const;

    /** Attempt to lock the mutex. Blocks until timeout passes or lock is acquired.
     * @param[in] timeout
     * @return success result
     */
    bool lock(TickType_t timeout) const override { return acquire(timeout) == TtStatusOk; }

    /** Attempt to unlock the mutex.
     * @return success result
     */
    bool unlock() const override { return release() == TtStatusOk; }

    /** @return the owner of the thread */
    ThreadId getOwner() const;
};

} // namespace
