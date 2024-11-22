/**
 * @file mutex.h
 * Mutex
 */
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

typedef enum {
    MutexTypeNormal,
    MutexTypeRecursive,
} MutexType;

class Mutex {
private:
    SemaphoreHandle_t semaphore;
    MutexType type;
public:
    Mutex(MutexType type);
    ~Mutex();

    TtStatus acquire(uint32_t timeout) const;
    TtStatus release() const;
    ThreadId getOwner() const;
};

/** Allocate Mutex
 *
 * @param[in]  type  The mutex type
 *
 * @return     pointer to Mutex instance
 */

[[deprecated("use class")]]
Mutex* tt_mutex_alloc(MutexType type);

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
