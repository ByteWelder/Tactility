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

static inline SemaphoreHandle_t createSemaphoreHandle(Mutex::Type type) {
    switch (type) {
        case Mutex::Type::Normal:
            return xSemaphoreCreateMutex();
        case Mutex::Type::Recursive:
            return xSemaphoreCreateRecursiveMutex();
        default:
            tt_crash("Mutex type unknown/corrupted");
    }
}

Mutex::Mutex(Type type) : handle(createSemaphoreHandle(type)), type(type) {
    tt_mutex_info(data, "alloc");
    assert(handle != nullptr);
}

Mutex::~Mutex() {
    handle = nullptr; // If the mutex is used after release, this might help debugging
}

TtStatus Mutex::acquire(TickType_t timeout) const {
    assert(!TT_IS_IRQ_MODE());
    assert(handle != nullptr);
    tt_mutex_info(mutex, "acquire");

    switch (type) {
        case Type::Normal:
            if (xSemaphoreTake(handle.get(), timeout) != pdPASS) {
                if (timeout != 0U) {
                    return TtStatusErrorTimeout;
                } else {
                    return TtStatusErrorResource;
                }
            } else {
                return TtStatusOk;
            }
        case Type::Recursive:
            if (xSemaphoreTakeRecursive(handle.get(), timeout) != pdPASS) {
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
    assert(handle != nullptr);
    tt_mutex_info(mutex, "release");

    switch (type) {
        case Type::Normal: {
            if (xSemaphoreGive(handle.get()) != pdPASS) {
                return TtStatusErrorResource;
            } else {
                return TtStatusOk;
            }
        }
        case Type::Recursive:
            if (xSemaphoreGiveRecursive(handle.get()) != pdPASS) {
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
    assert(handle != nullptr);
    return (ThreadId)xSemaphoreGetMutexHolder(handle.get());
}

} // namespace
