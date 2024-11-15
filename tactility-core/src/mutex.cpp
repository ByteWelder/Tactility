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

typedef struct {
    SemaphoreHandle_t handle;
    MutexType type;
} MutexData;

#define MUTEX_DEBUGGING false

#if MUTEX_DEBUGGING
#define TAG "mutex"
void tt_mutex_info(Mutex mutex, const char* label) {
    MutexData* data = (MutexData*)mutex;
    if (data == NULL) {
        TT_LOG_I(TAG, "mutex %s: is NULL", label);
    } else {
        TT_LOG_I(TAG, "mutex %s: handle=%0X type=%d owner=%0x", label, data->handle, data->type, tt_mutex_get_owner(mutex));
    }
}
#else
#define tt_mutex_info(mutex, text)
#endif

Mutex* tt_mutex_alloc(MutexType type) {
    tt_assert(!TT_IS_IRQ_MODE());
    auto data = static_cast<MutexData*>(malloc(sizeof(MutexData)));

    data->type = type;

    switch (type) {
        case MutexTypeNormal:
            data->handle = xSemaphoreCreateMutex();
            break;
        case MutexTypeRecursive:
            data->handle = xSemaphoreCreateRecursiveMutex();
            break;
        default:
            tt_crash("mutex type unknown/corrupted");
    }

    tt_check(data->handle != NULL);
    tt_mutex_info(data, "alloc  ");
    return (Mutex*)data;
}

void tt_mutex_free(Mutex* mutex) {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(mutex);

    auto* data = static_cast<MutexData*>(mutex);
    vSemaphoreDelete(data->handle);
    data->handle = nullptr; // If the mutex is used after release, this might help debugging
    data->type = static_cast<MutexType>(0xBAADF00D); // Set to an invalid value
    free(data);
}

TtStatus tt_mutex_acquire(Mutex* mutex, uint32_t timeout) {
    tt_assert(mutex);
    tt_assert(!TT_IS_IRQ_MODE());
    auto* data = static_cast<MutexData*>(mutex);
    tt_assert(data->handle);
    TtStatus status = TtStatusOk;

    tt_mutex_info(mutex, "acquire");

    switch (data->type) {
        case MutexTypeNormal:
            if (xSemaphoreTake(data->handle, timeout) != pdPASS) {
                if (timeout != 0U) {
                    status = TtStatusErrorTimeout;
                } else {
                    status = TtStatusErrorResource;
                }
            }
            break;
        case MutexTypeRecursive:
            if (xSemaphoreTakeRecursive(data->handle, timeout) != pdPASS) {
                if (timeout != 0U) {
                    status = TtStatusErrorTimeout;
                } else {
                    status = TtStatusErrorResource;
                }
            }
            break;
        default:
            tt_crash("mutex type unknown/corrupted");
    }

    return status;
}

TtStatus tt_mutex_release(Mutex* mutex) {
    tt_assert(mutex);
    assert(!TT_IS_IRQ_MODE());
    auto* data = static_cast<MutexData*>(mutex);
    tt_assert(data->handle);
    TtStatus status = TtStatusOk;

    tt_mutex_info(mutex, "release");

    switch (data->type) {
        case MutexTypeNormal: {
            if (xSemaphoreGive(data->handle) != pdPASS) {
                status = TtStatusErrorResource;
            }
            break;
        }
        case MutexTypeRecursive:
            if (xSemaphoreGiveRecursive(data->handle) != pdPASS) {
                status = TtStatusErrorResource;
            }
            break;
        default:
            tt_crash("mutex type unknown/corrupted %d");
    }

    return status;
}

ThreadId tt_mutex_get_owner(Mutex* mutex) {
    tt_assert(mutex != NULL);
    tt_assert(!TT_IS_IRQ_MODE());
    auto* data = static_cast<MutexData*>(mutex);
    tt_assert(data->handle);
    return (ThreadId)xSemaphoreGetMutexHolder(data->handle);
}
