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

bool Mutex::lock(TickType_t timeout) const {
    assert(!TT_IS_IRQ_MODE());
    assert(handle != nullptr);
    tt_mutex_info(mutex, "acquire");

    switch (type) {
        case Type::Normal:
            return xSemaphoreTake(handle.get(), timeout) == pdPASS;
        case Type::Recursive:
            return xSemaphoreTakeRecursive(handle.get(), timeout) == pdPASS;
        default:
            tt_crash();
    }
}

bool Mutex::unlock() const {
    assert(!TT_IS_IRQ_MODE());
    assert(handle != nullptr);
    tt_mutex_info(mutex, "release");

    switch (type) {
        case Type::Normal:
            return xSemaphoreGive(handle.get()) == pdPASS;
        case Type::Recursive:
            return xSemaphoreGiveRecursive(handle.get()) == pdPASS;
        default:
            tt_crash();
    }
}

ThreadId Mutex::getOwner() const {
    assert(!TT_IS_IRQ_MODE());
    assert(handle != nullptr);
    return (ThreadId)xSemaphoreGetMutexHolder(handle.get());
}

} // namespace
