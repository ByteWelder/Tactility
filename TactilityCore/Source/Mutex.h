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

class ScopedMutexUsage;

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

/** Allocate Mutex
 * @param[in] type The mutex type
 * @return pointer to Mutex instance
 */

[[deprecated("use class")]]
Mutex* tt_mutex_alloc(Mutex::Type type);

/** Free Mutex
 * @param[in] mutex The Mutex instance
 */
[[deprecated("use class")]]
void tt_mutex_free(Mutex* mutex);

/** Acquire mutex
 * @param[in] mutex
 * @param[in] timeout
 * @return the status result
 */
[[deprecated("use class")]]
TtStatus tt_mutex_acquire(Mutex* mutex, TickType_t timeout);

/** Release mutex
 * @param[in] mutex The Mutex instance
 * @return the status result
 */
[[deprecated("use class")]]
TtStatus tt_mutex_release(Mutex* mutex);

/** Get mutex owner thread id
 * @param[in] mutex The Mutex instance
 * @return The thread identifier.
 */
[[deprecated("use class")]]
ThreadId tt_mutex_get_owner(Mutex* mutex);

} // namespace
