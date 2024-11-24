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

Mutex::Mutex(MutexType type) : type(type) {
    tt_mutex_info(data, "alloc");
    switch (type) {
        case MutexTypeNormal:
            semaphore = xSemaphoreCreateMutex();
            break;
        case MutexTypeRecursive:
            semaphore = xSemaphoreCreateRecursiveMutex();
            break;
        default:
            tt_crash("Mutex type unknown/corrupted");
    }

    tt_check(semaphore != nullptr);
}

Mutex::~Mutex() {
    tt_assert(!TT_IS_IRQ_MODE());
    vSemaphoreDelete(semaphore);
    semaphore = nullptr; // If the mutex is used after release, this might help debugging
}

TtStatus Mutex::acquire(uint32_t timeout) const {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);
    TtStatus status = TtStatusOk;

    tt_mutex_info(mutex, "acquire");

    switch (type) {
        case MutexTypeNormal:
            if (xSemaphoreTake(semaphore, timeout) != pdPASS) {
                if (timeout != 0U) {
                    status = TtStatusErrorTimeout;
                } else {
                    status = TtStatusErrorResource;
                }
            }
            break;
        case MutexTypeRecursive:
            if (xSemaphoreTakeRecursive(semaphore, timeout) != pdPASS) {
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

TtStatus Mutex::release() const {
    assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);
    TtStatus status = TtStatusOk;

    tt_mutex_info(mutex, "release");

    switch (type) {
        case MutexTypeNormal: {
            if (xSemaphoreGive(semaphore) != pdPASS) {
                status = TtStatusErrorResource;
            }
            break;
        }
        case MutexTypeRecursive:
            if (xSemaphoreGiveRecursive(semaphore) != pdPASS) {
                status = TtStatusErrorResource;
            }
            break;
        default:
            tt_crash("mutex type unknown/corrupted");
    }

    return status;
}

ThreadId Mutex::getOwner() const {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);
    return (ThreadId)xSemaphoreGetMutexHolder(semaphore);
}


Mutex* tt_mutex_alloc(MutexType type) {
    return new Mutex(type);
}

void tt_mutex_free(Mutex* mutex) {
    delete mutex;
}

TtStatus tt_mutex_acquire(Mutex* mutex, uint32_t timeout) {
    return mutex-> acquire(timeout);
}

TtStatus tt_mutex_release(Mutex* mutex) {
    return mutex->release();

}

ThreadId tt_mutex_get_owner(Mutex* mutex) {
    return mutex->getOwner();
}

} // namespace
