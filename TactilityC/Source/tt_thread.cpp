#include "tt_thread.h"
#include <Tactility/Thread.h>

extern "C" {

#define HANDLE_AS_THREAD(handle) ((tt::Thread*)(handle))

ThreadHandle tt_thread_alloc() {
    return new tt::Thread();
}

ThreadHandle tt_thread_alloc_ext(
    const char* name,
    uint32_t stackSize,
    ThreadCallback callback,
    void* _Nullable callbackContext
) {
    return new tt::Thread(
        name,
        stackSize,
        [callback, callbackContext] {
            return callback(callbackContext);
        }
    );
}

void tt_thread_free(ThreadHandle handle) {
    delete HANDLE_AS_THREAD(handle);
}

void tt_thread_set_name(ThreadHandle handle, const char* name) {
    HANDLE_AS_THREAD(handle)->setName(name);
}

void tt_thread_set_stack_size(ThreadHandle handle, size_t size) {
    HANDLE_AS_THREAD(handle)->setStackSize(size);
}

void tt_thread_set_affinity(ThreadHandle handle, int affinity) {
    HANDLE_AS_THREAD(handle)->setAffinity(affinity);
}

void tt_thread_set_callback(ThreadHandle handle, ThreadCallback callback, void* _Nullable callbackContext) {
    HANDLE_AS_THREAD(handle)->setMainFunction([callback, callbackContext]() {
        return callback(callbackContext);
    });
}

void tt_thread_set_priority(ThreadHandle handle, ThreadPriority priority) {
    HANDLE_AS_THREAD(handle)->setPriority((tt::Thread::Priority)priority);
}

void tt_thread_set_state_callback(ThreadHandle handle, ThreadStateCallback callback, void* _Nullable callbackContext) {
    HANDLE_AS_THREAD(handle)->setStateCallback((tt::Thread::StateCallback)callback, callbackContext);
}

ThreadState tt_thread_get_state(ThreadHandle handle) {
    return (ThreadState)HANDLE_AS_THREAD(handle)->getState();
}

void tt_thread_start(ThreadHandle handle) {
    HANDLE_AS_THREAD(handle)->start();
}

bool tt_thread_join(ThreadHandle handle, TickType_t timeout) {
    return HANDLE_AS_THREAD(handle)->join(timeout);
}

ThreadId tt_thread_get_id(ThreadHandle handle) {
    return HANDLE_AS_THREAD(handle)->getId();
}

int32_t tt_thread_get_return_code(ThreadHandle handle) {
    return HANDLE_AS_THREAD(handle)->getReturnCode();
}

}