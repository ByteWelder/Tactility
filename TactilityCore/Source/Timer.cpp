#include "Timer.h"

#include <utility>
#include "Check.h"
#include "RtosCompat.h"

namespace tt {

void Timer::onCallback(TimerHandle_t hTimer) {
    auto* timer = static_cast<Timer*>(pvTimerGetTimerID(hTimer));

    if (timer != nullptr) {
        timer->callback(timer->callbackContext);
    }
}

static inline TimerHandle_t createTimer(Timer::Type type, void* timerId, TimerCallbackFunction_t callback) {
    assert(timerId != nullptr);
    assert(callback != nullptr);

    UBaseType_t reload;
    if (type == Timer::Type::Once) {
        reload = pdFALSE;
    } else {
        reload = pdTRUE;
    }

    return xTimerCreate(nullptr, portMAX_DELAY, (BaseType_t)reload, timerId, callback);
}

Timer::Timer(Type type, Callback callback, std::shared_ptr<void> callbackContext) :
    callback(callback),
    callbackContext(std::move(callbackContext)),
    handle(createTimer(type, this, onCallback))
{
    assert(!TT_IS_ISR());
    assert(handle != nullptr);
}

Timer::~Timer() {
    assert(!TT_IS_ISR());
}

bool Timer::start(TickType_t interval) {
    assert(!TT_IS_ISR());
    assert(interval < portMAX_DELAY);
    return xTimerChangePeriod(handle.get(), interval, portMAX_DELAY) == pdPASS;
}

bool Timer::restart(TickType_t interval) {
    assert(!TT_IS_ISR());
    assert(interval < portMAX_DELAY);
    return xTimerChangePeriod(handle.get(), interval, portMAX_DELAY) == pdPASS &&
        xTimerReset(handle.get(), portMAX_DELAY) == pdPASS;
}

bool Timer::stop() {
    assert(!TT_IS_ISR());
    return xTimerStop(handle.get(), portMAX_DELAY) == pdPASS;
}

bool Timer::isRunning() {
    assert(!TT_IS_ISR());
    return xTimerIsTimerActive(handle.get()) == pdTRUE;
}

TickType_t Timer::getExpireTime() {
    assert(!TT_IS_ISR());
    return xTimerGetExpiryTime(handle.get());
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
    assert(!TT_IS_ISR());

    TaskHandle_t task_handle = xTimerGetTimerDaemonTaskHandle();
    assert(task_handle); // Don't call this method before timer task start

    vTaskPrioritySet(task_handle, static_cast<UBaseType_t>(priority));
}

} // namespace
