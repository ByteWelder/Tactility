#include "Mutex.h"

#include "Check.h"
#include "CoreDefines.h"
#include "Log.h"

namespace tt {

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

Mutex::Mutex(Type type) : type(type) {
    tt_mutex_info(data, "alloc");
    switch (type) {
        case Type::Normal:
            semaphore = xSemaphoreCreateMutex();
            break;
        case Type::Recursive:
            semaphore = xSemaphoreCreateRecursiveMutex();
            break;
        default:
            tt_crash("Mutex type unknown/corrupted");
    }

    assert(semaphore != nullptr);
}

Mutex::~Mutex() {
    assert(!TT_IS_IRQ_MODE());
    vSemaphoreDelete(semaphore);
    semaphore = nullptr; // If the mutex is used after release, this might help debugging
}

TtStatus Mutex::acquire(TickType_t timeout) const {
    assert(!TT_IS_IRQ_MODE());
    assert(semaphore != nullptr);

    tt_mutex_info(mutex, "acquire");

    switch (type) {
        case Type::Normal:
            if (xSemaphoreTake(semaphore, timeout) != pdPASS) {
                if (timeout != 0U) {
                    return TtStatusErrorTimeout;
                } else {
                    return TtStatusErrorResource;
                }
            } else {
                return TtStatusOk;
            }
        case Type::Recursive:
            if (xSemaphoreTakeRecursive(semaphore, timeout) != pdPASS) {
                if (timeout != 0U) {
                    return TtStatusErrorTimeout;
                } else {
                    return TtStatusErrorResource;
                }
            } else {
                return TtStatusOk;
            }
        default:
            tt_crash("mutex type unknown/corrupted");
    }
}

TtStatus Mutex::release() const {
    assert(!TT_IS_IRQ_MODE());
    assert(semaphore);
    tt_mutex_info(mutex, "release");

    switch (type) {
        case Type::Normal: {
            if (xSemaphoreGive(semaphore) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                return TtStatusOk;
            }
        }
        case Type::Recursive:
            if (xSemaphoreGiveRecursive(semaphore) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                return TtStatusOk;
            }
        default:
            tt_crash("mutex type unknown/corrupted");
    }
}

ThreadId Mutex::getOwner() const {
    assert(!TT_IS_IRQ_MODE());
    assert(semaphore);
    return (ThreadId)xSemaphoreGetMutexHolder(semaphore);
}

} // namespace
