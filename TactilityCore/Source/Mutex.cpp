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
        case TypeNormal:
            semaphore = xSemaphoreCreateMutex();
            break;
        case TypeRecursive:
            semaphore = xSemaphoreCreateRecursiveMutex();
            break;
        default:
            tt_crash("Mutex type unknown/corrupted");
    }

    tt_assert(semaphore != nullptr);
}

Mutex::~Mutex() {
    tt_assert(!TT_IS_IRQ_MODE());
    vSemaphoreDelete(semaphore);
    semaphore = nullptr; // If the mutex is used after release, this might help debugging
}

TtStatus Mutex::acquire(uint32_t timeout) const {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);

    tt_mutex_info(mutex, "acquire");

    switch (type) {
        case TypeNormal:
            if (xSemaphoreTake(semaphore, timeout) != pdPASS) {
                if (timeout != 0U) {
                    return TtStatusErrorTimeout;
                } else {
                    return TtStatusErrorResource;
                }
            } else {
                return TtStatusOk;
            }
            break;
        case TypeRecursive:
            if (xSemaphoreTakeRecursive(semaphore, timeout) != pdPASS) {
                if (timeout != 0U) {
                    return TtStatusErrorTimeout;
                } else {
                    return TtStatusErrorResource;
                }
            } else {
                return TtStatusOk;
            }
            break;
        default:
            tt_crash("mutex type unknown/corrupted");
    }
}

TtStatus Mutex::release() const {
    assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);
    tt_mutex_info(mutex, "release");

    switch (type) {
        case TypeNormal: {
            if (xSemaphoreGive(semaphore) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                return TtStatusOk;
            }
            break;
        }
        case TypeRecursive:
            if (xSemaphoreGiveRecursive(semaphore) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                return TtStatusOk;
            }
            break;
        default:
            tt_crash("mutex type unknown/corrupted");
    }
}

ThreadId Mutex::getOwner() const {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(semaphore);
    return (ThreadId)xSemaphoreGetMutexHolder(semaphore);
}

Mutex* tt_mutex_alloc(Mutex::Type type) {
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
