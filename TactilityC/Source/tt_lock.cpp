#include <tt_lock.h>
#include <tt_lock_private.h>

extern "C" {

bool tt_lock_acquire(LockHandle handle, TickType timeout) {
    auto holder = static_cast<LockHolder*>(handle);
    return holder->lock->lock(timeout);
}

bool tt_lock_release(LockHandle handle) {
    auto holder = static_cast<LockHolder*>(handle);
    return holder->lock->unlock();
}

void tt_lock_free(LockHandle handle) {
    auto holder = static_cast<LockHolder*>(handle);
    delete holder;
}

}