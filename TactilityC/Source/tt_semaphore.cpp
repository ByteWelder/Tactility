#include "tt_semaphore.h"
#include "Semaphore.h"

extern "C" {

#define HANDLE_AS_SEMAPHORE(handle) ((tt::Semaphore*)(handle))

SemaphoreHandle tt_semaphore_alloc(uint32_t maxCount, TickType_t initialCount) {
    return new tt::Semaphore(maxCount, initialCount);
}

void tt_semaphore_free(SemaphoreHandle handle) {
    delete HANDLE_AS_SEMAPHORE(handle);
}

bool tt_semaphore_acquire(SemaphoreHandle handle, TickType_t timeoutTicks) {
    return HANDLE_AS_SEMAPHORE(handle)->acquire(timeoutTicks);
}

bool tt_semaphore_release(SemaphoreHandle handle) {
    return HANDLE_AS_SEMAPHORE(handle)->release();
}

uint32_t tt_semaphore_get_count(SemaphoreHandle handle) {
    return HANDLE_AS_SEMAPHORE(handle)->getCount();
}

}
