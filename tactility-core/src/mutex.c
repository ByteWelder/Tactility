#include "mutex.h"

#include "check.h"
#include "core_defines.h"
#include "log.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif


Mutex* tt_mutex_alloc(MutexType type) {
    tt_assert(!TT_IS_IRQ_MODE());

    SemaphoreHandle_t hMutex = NULL;

    if (type == MutexTypeNormal) {
        hMutex = xSemaphoreCreateMutex();
    } else if (type == MutexTypeRecursive) {
        hMutex = xSemaphoreCreateRecursiveMutex();
    } else {
        tt_crash("Programming error");
    }

    tt_check(hMutex != NULL);

    if (type == MutexTypeRecursive) {
        /* Set LSB as 'recursive mutex flag' */
        hMutex = (SemaphoreHandle_t)((uint32_t)hMutex | 1U);
    }

    /* Return mutex ID */
    return ((Mutex*)hMutex);
}

void tt_mutex_free(Mutex* instance) {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(instance);

    vSemaphoreDelete((SemaphoreHandle_t)((uint32_t)instance & ~1U));
}

TtStatus tt_mutex_acquire(Mutex* instance, uint32_t timeout) {
    SemaphoreHandle_t hMutex;
    TtStatus stat;
    uint32_t rmtx;

    hMutex = (SemaphoreHandle_t)((uint32_t)instance & ~1U);

    /* Extract recursive mutex flag */
    rmtx = (uint32_t)instance & 1U;

    stat = TtStatusOk;

    if (TT_IS_IRQ_MODE()) {
        stat = TtStatusErrorISR;
    } else if (hMutex == NULL) {
        stat = TtStatusErrorParameter;
    } else {
        if (rmtx != 0U) {
            if (xSemaphoreTakeRecursive(hMutex, timeout) != pdPASS) {
                if (timeout != 0U) {
                    stat = TtStatusErrorTimeout;
                } else {
                    stat = TtStatusErrorResource;
                }
            }
        } else {
            if (xSemaphoreTake(hMutex, timeout) != pdPASS) {
                if (timeout != 0U) {
                    stat = TtStatusErrorTimeout;
                } else {
                    stat = TtStatusErrorResource;
                }
            }
        }
    }

    /* Return execution status */
    return (stat);
}

TtStatus tt_mutex_release(Mutex* instance) {
    SemaphoreHandle_t hMutex;
    TtStatus stat;
    uint32_t rmtx;

    hMutex = (SemaphoreHandle_t)((uint32_t)instance & ~1U);

    /* Extract recursive mutex flag */
    rmtx = (uint32_t)instance & 1U;

    stat = TtStatusOk;

    if (TT_IS_IRQ_MODE()) {
        stat = TtStatusErrorISR;
    } else if (hMutex == NULL) {
        stat = TtStatusErrorParameter;
    } else {
        if (rmtx != 0U) {
            if (xSemaphoreGiveRecursive(hMutex) != pdPASS) {
                stat = TtStatusErrorResource;
            }
        } else {
            if (xSemaphoreGive(hMutex) != pdPASS) {
                stat = TtStatusErrorResource;
            }
        }
    }

    /* Return execution status */
    return (stat);
}

ThreadId tt_mutex_get_owner(Mutex* instance) {
    SemaphoreHandle_t hMutex;
    ThreadId owner;

    hMutex = (SemaphoreHandle_t)((uint32_t)instance & ~1U);

    if ((TT_IS_IRQ_MODE()) || (hMutex == NULL)) {
        owner = 0;
    } else {
        owner = (ThreadId)xSemaphoreGetMutexHolder(hMutex);
    }

    /* Return owner thread ID */
    return (owner);
}
