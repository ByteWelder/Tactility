#include "Timer.h"

#include <utility>
#include "Check.h"
#include "RtosCompat.h"

namespace tt {

static void timer_callback(TimerHandle_t hTimer) {
    auto* timer = static_cast<Timer*>(pvTimerGetTimerID(hTimer));

    if (timer != nullptr) {
        timer->callback(timer->callbackContext);
    }
}

Timer::Timer(Type type, Callback callback, std::shared_ptr<void> callbackContext) {
    tt_assert((!TT_IS_ISR()) && (callback != nullptr));

    this->callback = callback;
    this->callbackContext = std::move(callbackContext);

    UBaseType_t reload;
    if (type == TypeOnce) {
        reload = pdFALSE;
    } else {
        reload = pdTRUE;
    }

    this->timerHandle = xTimerCreate(nullptr, portMAX_DELAY, (BaseType_t)reload, this, timer_callback);
    tt_assert(this->timerHandle);
}

Timer::~Timer() {
    tt_assert(!TT_IS_ISR());
    tt_check(xTimerDelete(timerHandle, portMAX_DELAY) == pdPASS);
}

bool Timer::start(TickType_t interval) {
    tt_assert(!TT_IS_ISR());
    tt_assert(interval < portMAX_DELAY);
    return xTimerChangePeriod(timerHandle, interval, portMAX_DELAY) == pdPASS;
}

bool Timer::restart(TickType_t interval) {
    tt_assert(!TT_IS_ISR());
    tt_assert(interval < portMAX_DELAY);
    return xTimerChangePeriod(timerHandle, interval, portMAX_DELAY) == pdPASS &&
        xTimerReset(timerHandle, portMAX_DELAY) == pdPASS;
}

bool Timer::stop() {
    tt_assert(!TT_IS_ISR());
    return xTimerStop(timerHandle, portMAX_DELAY) == pdPASS;
}

bool Timer::isRunning() {
    tt_assert(!TT_IS_ISR());
    return xTimerIsTimerActive(timerHandle) == pdTRUE;
}

TickType_t Timer::getExpireTime() {
    tt_assert(!TT_IS_ISR());
    return xTimerGetExpiryTime(timerHandle);
}

bool Timer::setPendingCallback(PendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeout) {
    if (TT_IS_ISR()) {
        assert(timeout == 0);
        return xTimerPendFunctionCallFromISR(callback, callbackContext, callbackArg, nullptr) == pdPASS;
    } else {
        return xTimerPendFunctionCall(callback, callbackContext, callbackArg, timeout) == pdPASS;
    }
}

void Timer::setThreadPriority(Thread::Priority priority)  {
    tt_assert(!TT_IS_ISR());

    TaskHandle_t task_handle = xTimerGetTimerDaemonTaskHandle();
    tt_assert(task_handle); // Don't call this method before timer task start

    vTaskPrioritySet(task_handle, static_cast<UBaseType_t>(priority));
}

} // namespace
