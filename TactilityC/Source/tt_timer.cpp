#include "tt_timer.h"
#include <Tactility/Timer.h>

struct TimerWrapper {
    std::unique_ptr<tt::Timer> timer;
};

#define HANDLE_TO_WRAPPER(handle) static_cast<TimerWrapper*>(handle)

extern "C" {

TimerHandle tt_timer_alloc(TimerType type, TickType_t ticks, TimerCallback callback, void* callbackContext) {
    auto wrapper = new TimerWrapper;
    wrapper->timer = std::make_unique<tt::Timer>(static_cast<tt::Timer::Type>(type), ticks, [callback, callbackContext](){ callback(callbackContext); });
    return wrapper;
}

void tt_timer_free(TimerHandle handle) {
    auto* wrapper = static_cast<TimerWrapper*>(handle);
    wrapper->timer = nullptr;
    delete wrapper;
}

bool tt_timer_start(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->start();
}

bool tt_timer_reset(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->reset();
}

bool tt_timer_reset_with_interval(TimerHandle handle, TickType_t interval) {
    return HANDLE_TO_WRAPPER(handle)->timer->reset(interval);
}

bool tt_timer_stop(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->stop();
}

bool tt_timer_is_running(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->isRunning();
}

uint32_t tt_timer_get_expiry_time(TimerHandle handle) {
    return HANDLE_TO_WRAPPER(handle)->timer->getExpiryTime();
}

bool tt_timer_set_pending_callback(TimerHandle handle, TimerPendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeoutTicks) {
    return HANDLE_TO_WRAPPER(handle)->timer->setPendingCallback(
        callback,
        callbackContext,
        callbackArg,
        timeoutTicks
    );
}

void tt_timer_set_thread_priority(TimerHandle handle, ThreadPriority priority) {
    HANDLE_TO_WRAPPER(handle)->timer->setCallbackPriority(static_cast<tt::Thread::Priority>(priority));
}

}
