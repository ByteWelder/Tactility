#pragma once

#include "core_types.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void Semaphore;

/** Allocate semaphore
 *
 * @param[in]  max_count      The maximum count
 * @param[in]  initial_count  The initial count
 *
 * @return     pointer to FuriSemaphore instance
 */
Semaphore* tt_semaphore_alloc(uint32_t max_count, uint32_t initial_count);

/** Free semaphore
 *
 * @param      instance  The pointer to FuriSemaphore instance
 */
void tt_semaphore_free(Semaphore* instance);

/** Acquire semaphore
 *
 * @param      instance  The pointer to FuriSemaphore instance
 * @param[in]  timeout   The timeout
 *
 * @return     The furi status.
 */
TtStatus tt_semaphore_acquire(Semaphore* instance, uint32_t timeout);

/** Release semaphore
 *
 * @param      instance  The pointer to FuriSemaphore instance
 *
 * @return     The furi status.
 */
TtStatus tt_semaphore_release(Semaphore* instance);

/** Get semaphore count
 *
 * @param      instance  The pointer to FuriSemaphore instance
 *
 * @return     Semaphore count
 */
uint32_t tt_semaphore_get_count(Semaphore* instance);

#ifdef __cplusplus
}
#endif
