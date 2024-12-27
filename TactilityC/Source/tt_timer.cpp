#include "tt_timer.h"
#include <Timer.h>

struct TimerWrapper {
    std::unique_ptr<tt::Timer> timer;
    TimerCallback callback;
    void* _Nullable callbackContext;
};

extern "C" {


static void callbackWrapper(std::shared_ptr<void> wrapper) {
    auto timer_wrapper = (TimerWrapper*)wrapper.get();
    timer_wrapper->callback(timer_wrapper->callbackContext);
}

TimerHandle tt_timer_alloc(TimerType type, TimerCallback callback, void* callbackContext) {
    auto wrapper = std::make_shared<TimerWrapper>();
    wrapper->callback = callback;
    wrapper->callbackContext = callbackContext;
    wrapper->timer = std::make_unique<tt::Timer>((tt::Timer::Type)type, callbackWrapper, wrapper);
    return wrapper.get();
}

void tt_timer_free(TimerHandle handle) {
    auto* wrapper = (TimerWrapper*)handle;
    wrapper->timer = nullptr;
    delete wrapper;
}

bool tt_timer_start(TimerHandle handle, uint32_t intervalTicks) {
    return ((TimerWrapper*)handle)->timer->start(intervalTicks);
}

bool tt_timer_restart(TimerHandle handle, uint32_t intervalTicks) {
    return ((TimerWrapper*)handle)->timer->restart(intervalTicks);
}

bool tt_timer_stop(TimerHandle handle) {
    return ((TimerWrapper*)handle)->timer->stop();
}

bool tt_timer_is_running(TimerHandle handle) {
    return ((TimerWrapper*)handle)->timer->isRunning();
}

uint32_t tt_timer_get_expire_time(TimerHandle handle) {
    return ((TimerWrapper*)handle)->timer->getExpireTime();
}

bool tt_timer_set_pending_callback(TimerHandle handle, TimerPendingCallback callback, void* callbackContext, uint32_t arg) {
    return ((TimerWrapper*)handle)->timer->setPendingCallback(
        callback,
        callbackContext,
        arg
    );
}

void tt_timer_set_thread_priority(TimerHandle handle, TimerThreadPriority priority) {
    ((TimerWrapper*)handle)->timer->setThreadPriority((tt::Timer::ThreadPriority)priority);
}

}

