/**
 * @file mutex.h
 * Mutex
 */
#pragma once

#include "core_types.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @param      instance  The pointer to Mutex instance
 */
void tt_mutex_free(Mutex* instance);

/** Acquire mutex
 *
 * @param      instance  The pointer to Mutex instance
 * @param[in]  timeout   The timeout
 *
 * @return     The furi status.
 */
TtStatus tt_mutex_acquire(Mutex* instance, uint32_t timeout);

/** Release mutex
 *
 * @param      instance  The pointer to Mutex instance
 *
 * @return     The furi status.
 */
TtStatus tt_mutex_release(Mutex* instance);

/** Get mutex owner thread id
 *
 * @param      instance  The pointer to Mutex instance
 *
 * @return     The furi thread identifier.
 */
ThreadId tt_mutex_get_owner(Mutex* instance);

#ifdef __cplusplus
}
#endif
