/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "core_types.h"
#include "thread.h"

typedef enum {
    MutexTypeNormal,
    MutexTypeRecursive,
} MutexType;

typedef void Mutex;

/** Allocate Mutex
 *
 * @param[in]  type  The mutex type
 *
 * @return     pointer to Mutex instance
 */
Mutex* tt_mutex_alloc(MutexType type);

/** Free Mutex
 *
 * @param      mutex  The Mutex instance
 */
void tt_mutex_free(Mutex* mutex);

/** Acquire mutex
 *
 * @param      mutex  The Mutex instance
 * @param[in]  timeout   The timeout
 *
 * @return     The status.
 */
TtStatus tt_mutex_acquire(Mutex* mutex, uint32_t timeout);

/** Release mutex
 *
 * @param      mutex  The Mutex instance
 *
 * @return     The status.
 */
TtStatus tt_mutex_release(Mutex* mutex);

/** Get mutex owner thread id
 *
 * @param      mutex  The Mutex instance
 *
 * @return     The thread identifier.
 */
ThreadId tt_mutex_get_owner(Mutex* mutex);
