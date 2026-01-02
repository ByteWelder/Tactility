#include <Tactility/Mutex.h>
#include <Tactility/RecursiveMutex.h>
#include <Tactility/file/File.h>
#include <tt_lock.h>
#include <tt_lock_private.h>

#define HANDLE_AS_LOCK(handle) (static_cast<LockHolder*>(handle)->lock)

extern "C" {

LockHandle tt_lock_alloc_mutex(TtMutexType type) {
    auto* lock_holder = new LockHolder();
    switch (type) {
        case MutexTypeNormal:
            lock_holder->lock = std::make_shared<tt::Mutex>();
            break;
        case MutexTypeRecursive:
            lock_holder->lock = std::make_shared<tt::RecursiveMutex>();
            break;
        default:
            tt_crash("Type not supported");
    }
    return lock_holder;
}

LockHandle tt_lock_alloc_for_path(const char* path) {
    const auto lock = tt::file::getLock(path);
    return new LockHolder(lock);
}

bool tt_lock_acquire(LockHandle handle, TickType timeout) {
    return HANDLE_AS_LOCK(handle)->lock(timeout);
}

void tt_lock_release(LockHandle handle) {
    HANDLE_AS_LOCK(handle)->unlock();
}

void tt_lock_free(LockHandle handle) {
    const auto holder = static_cast<LockHolder*>(handle);
    delete holder;
}

}