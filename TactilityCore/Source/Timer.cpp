#include "Timer.h"

#include <utility>
#include "Check.h"
#include "kernel/Kernel.h"
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

bool Timer::start(uint32_t intervalTicks) {
    tt_assert(!TT_IS_ISR());
    tt_assert(intervalTicks < portMAX_DELAY);
    return xTimerChangePeriod(timerHandle, intervalTicks, portMAX_DELAY) == pdPASS;
}

bool Timer::restart(uint32_t intervalTicks) {
    tt_assert(!TT_IS_ISR());
    tt_assert(intervalTicks < portMAX_DELAY);
    return xTimerChangePeriod(timerHandle, intervalTicks, portMAX_DELAY) == pdPASS &&
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

uint32_t Timer::getExpireTime() {
    tt_assert(!TT_IS_ISR());
    return (uint32_t)xTimerGetExpiryTime(timerHandle);
}

bool Timer::setPendingCallback(PendingCallback callback, void* callbackContext, uint32_t arg) {
    if (TT_IS_ISR()) {
        return xTimerPendFunctionCallFromISR(callback, callbackContext, arg, nullptr) == pdPASS;
    } else {
        return xTimerPendFunctionCall(callback, callbackContext, arg, TtWaitForever) == pdPASS;
    }
}

void Timer::setThreadPriority(ThreadPriority priority)  {
    tt_assert(!TT_IS_ISR());

    TaskHandle_t task_handle = xTimerGetTimerDaemonTaskHandle();
    tt_assert(task_handle); // Don't call this method before timer task start

    if (priority == TimerThreadPriorityNormal) {
        vTaskPrioritySet(task_handle, configTIMER_TASK_PRIORITY);
    } else if (priority == TimerThreadPriorityElevated) {
        vTaskPrioritySet(task_handle, configMAX_PRIORITIES - 1);
    } else {
        tt_crash("Unsupported timer priority");
    }
}

} // namespace
