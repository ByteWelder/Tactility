#include "tt_timer.h"
#include <Tactility/Timer.h>

struct TimerWrapper {
    std::unique_ptr<tt::Timer> timer;
};

#define HANDLE_TO_WRAPPER(handle) static_cast<TimerWrapper*>(handle)

extern "C" {

TimerHandle tt_timer_alloc(TimerType type, TimerCallback callback, void* callbackContext) {
    auto wrapper = new TimerWrapper;
    wrapper->timer = std::make_unique<tt::Timer>(static_cast<tt::Timer::Type>(type), [callback, callbackContext](){ callback(callbackContext); });
    return wrapper;
}

void tt_timer_free(TimerHandle handle) {
    auto* wrapper = static_cast<TimerWrapper*>(handle);
    wrapper->timer = nullptr;
    delete wrapper;
}

bool tt_timer_start(TimerHandle handle, TickType_t intervalTicks) {
    return HANDLE_TO_WRAPPER(handle)->timer->start(intervalTicks);
}

bool tt_timer_restart(TimerHandle handle, TickType_t intervalTicks) {
    return HANDLE_TO_WRAPPER(handle)->timer->restart(intervalTicks);
}

bool tt_timer_stop(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->stop();
}

bool tt_timer_is_running(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->isRunning();
}

uint32_t tt_timer_get_expire_time(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->getExpireTime();
}

bool tt_timer_set_pending_callback(TimerHandle handle, TimerPendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeoutTicks) {
    return HANDLE_TO_WRAPPER(handle)->timer->setPendingCallback(
        callback,
        callbackContext,
        callbackArg,
        (TickType_t)timeoutTicks
    );
}

void tt_timer_set_thread_priority(TimerHandle handle, ThreadPriority priority) {
    HANDLE_TO_WRAPPER(handle)->timer->setThreadPriority(static_cast<tt::Thread::Priority>(priority));
}

}
