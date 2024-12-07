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
    tt_assert((kernel::isIrq() == 0U) && (callback != nullptr));

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
    tt_assert(!kernel::isIrq());
    tt_check(xTimerDelete(timerHandle, portMAX_DELAY) == pdPASS);
}

TtStatus Timer::start(uint32_t ticks) {
    tt_assert(!kernel::isIrq());
    tt_assert(ticks < portMAX_DELAY);

    if (xTimerChangePeriod(timerHandle, ticks, portMAX_DELAY) == pdPASS) {
        return TtStatusOk;
    } else {
        return TtStatusErrorResource;
    }
}

TtStatus Timer::restart(uint32_t ticks) {
    tt_assert(!kernel::isIrq());
    tt_assert(ticks < portMAX_DELAY);

    if (xTimerChangePeriod(timerHandle, ticks, portMAX_DELAY) == pdPASS &&
        xTimerReset(timerHandle, portMAX_DELAY) == pdPASS) {
        return TtStatusOk;
    } else {
        return TtStatusErrorResource;
    }
}

TtStatus Timer::stop() {
    tt_assert(!kernel::isIrq());
    tt_check(xTimerStop(timerHandle, portMAX_DELAY) == pdPASS);
    return TtStatusOk;
}

bool Timer::isRunning() {
    tt_assert(!kernel::isIrq());
    return xTimerIsTimerActive(timerHandle) == pdTRUE;
}

uint32_t Timer::getExpireTime() {
    tt_assert(!kernel::isIrq());
    return (uint32_t)xTimerGetExpiryTime(timerHandle);
}

void Timer::pendingCallback(PendingCallback callback, void* callbackContext, uint32_t arg) {
    BaseType_t ret = pdFAIL;
    if (kernel::isIrq()) {
        ret = xTimerPendFunctionCallFromISR(callback, callbackContext, arg, nullptr);
    } else {
        ret = xTimerPendFunctionCall(callback, callbackContext, arg, TtWaitForever);
    }
    tt_assert(ret == pdPASS);
}

void Timer::setThreadPriority(TimerThreadPriority priority)  {
    tt_assert(!kernel::isIrq());

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
