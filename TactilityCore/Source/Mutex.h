/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "CoreTypes.h"
#include "Thread.h"
#include "RtosCompatSemaphore.h"
#include "Check.h"
#include <memory>

namespace tt {

class ScopedMutexUsage;

/**
 * Wrapper for FreeRTOS xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex
 * Can be used in IRQ mode (within ISR context)
 */
class Mutex {

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
    ~Mutex();

    TtStatus acquire(uint32_t timeout) const;
    TtStatus release() const;
    ThreadId getOwner() const;

    std::unique_ptr<ScopedMutexUsage> scoped() const;
};

class ScopedMutexUsage {

    const Mutex& mutex;
    bool acquired = false;

public:

    ScopedMutexUsage(const Mutex& mutex) : mutex(mutex) {}

    ~ScopedMutexUsage() {
        if (acquired) {
            tt_check(mutex.release() == TtStatusOk);
        }
    }

    bool acquire(uint32_t timeout) {
        TtStatus result = mutex.acquire(timeout);
        acquired = (result == TtStatusOk);
        return acquired;
    }
};

/** Allocate Mutex
 *
 * @param[in]  type  The mutex type
 *
 * @return     pointer to Mutex instance
 */

[[deprecated("use class")]]
Mutex* tt_mutex_alloc(Mutex::Type type);

/** Free Mutex
 *
 * @param      mutex  The Mutex instance
 */
[[deprecated("use class")]]
void tt_mutex_free(Mutex* mutex);

/** Acquire mutex
 *
 * @param      mutex  The Mutex instance
 * @param[in]  timeout   The timeout
 *
 * @return     The status.
 */
[[deprecated("use class")]]
TtStatus tt_mutex_acquire(Mutex* mutex, uint32_t timeout);

/** Release mutex
 *
 * @param      mutex  The Mutex instance
 *
 * @return     The status.
 */
[[deprecated("use class")]]
TtStatus tt_mutex_release(Mutex* mutex);

/** Get mutex owner thread id
 *
 * @param      mutex  The Mutex instance
 *
 * @return     The thread identifier.
 */
[[deprecated("use class")]]
ThreadId tt_mutex_get_owner(Mutex* mutex);

} // namespace
