#include "tt_mutex.h"
#include <Tactility/Mutex.h>

extern "C" {

#define HANDLE_AS_MUTEX(handle) ((tt::Mutex*)(handle))

MutexHandle tt_mutex_alloc(enum TtMutexType type) {
    switch (type) {
        case TtMutexType::MUTEX_TYPE_NORMAL:
            return new tt::Mutex(tt::Mutex::Type::Normal);
        case TtMutexType::MUTEX_TYPE_RECURSIVE:
            return new tt::Mutex(tt::Mutex::Type::Recursive);
        default:
            tt_crash("Type not supported");
    }
}

void tt_mutex_free(MutexHandle handle) {
    delete HANDLE_AS_MUTEX(handle);
}

bool tt_mutex_lock(MutexHandle handle, TickType_t timeout) {
    return HANDLE_AS_MUTEX(handle)->lock((TickType_t)timeout);
}

bool tt_mutex_unlock(MutexHandle handle) {
    return HANDLE_AS_MUTEX(handle)->unlock();
}

}