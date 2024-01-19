#include "timer.h"
#include "check.h"
#include "kernel.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#else
#include "FreeRTOS.h"
#include "timers.h"
#endif

typedef struct {
    TimerCallback func;
    void* context;
} TimerCallback_t;

static void timer_callback(TimerHandle_t hTimer) {
    TimerCallback_t* callb;

    /* Retrieve pointer to callback function and context */
    callb = (TimerCallback_t*)pvTimerGetTimerID(hTimer);

    /* Remove dynamic allocation flag */
    callb = (TimerCallback_t*)((uint32_t)callb & ~1U);

    if (callb != NULL) {
        callb->func(callb->context);
    }
}

Timer* tt_timer_alloc(TimerCallback func, TimerType type, void* context) {
    tt_assert((tt_kernel_is_irq() == 0U) && (func != NULL));

    TimerHandle_t hTimer;
    TimerCallback_t* callb;
    UBaseType_t reload;

    hTimer = NULL;

    /* Dynamic memory allocation is available: if memory for callback and */
    /* its context is not provided, allocate it from dynamic memory pool */
    callb = (TimerCallback_t*)malloc(sizeof(TimerCallback_t));

    callb->func = func;
    callb->context = context;

    if (type == TimerTypeOnce) {
        reload = pdFALSE;
    } else {
        reload = pdTRUE;
    }

    /* Store callback memory dynamic allocation flag */
    callb = (TimerCallback_t*)((uint32_t)callb | 1U);
    // TimerCallback function is always provided as a callback and is used to call application
    // specified function with its context both stored in structure callb.
    // TODO: should we use pointer to function or function directly as-is?
    hTimer = xTimerCreate(NULL, portMAX_DELAY, reload, callb, timer_callback);
    tt_check(hTimer);

    /* Return timer ID */
    return ((Timer*)hTimer);
}

void tt_timer_free(Timer* instance) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t)instance;
    TimerCallback_t* callb;

    callb = (TimerCallback_t*)pvTimerGetTimerID(hTimer);

    tt_check(xTimerDelete(hTimer, portMAX_DELAY) == pdPASS);

    while (tt_timer_is_running(instance)) tt_delay_tick(2);

    if ((uint32_t)callb & 1U) {
        /* Callback memory was allocated from dynamic pool, clear flag */
        callb = (TimerCallback_t*)((uint32_t)callb & ~1U);

        /* Return allocated memory to dynamic pool */
        free(callb);
    }
}

TtStatus tt_timer_start(Timer* instance, uint32_t ticks) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);
    tt_assert(ticks < portMAX_DELAY);

    TimerHandle_t hTimer = (TimerHandle_t)instance;
    TtStatus stat;

    if (xTimerChangePeriod(hTimer, ticks, portMAX_DELAY) == pdPASS) {
        stat = TtStatusOk;
    } else {
        stat = TtStatusErrorResource;
    }

    /* Return execution status */
    return (stat);
}

TtStatus tt_timer_restart(Timer* instance, uint32_t ticks) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);
    tt_assert(ticks < portMAX_DELAY);

    TimerHandle_t hTimer = (TimerHandle_t)instance;
    TtStatus stat;

    if (xTimerChangePeriod(hTimer, ticks, portMAX_DELAY) == pdPASS &&
        xTimerReset(hTimer, portMAX_DELAY) == pdPASS) {
        stat = TtStatusOk;
    } else {
        stat = TtStatusErrorResource;
    }

    /* Return execution status */
    return (stat);
}

TtStatus tt_timer_stop(Timer* instance) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t)instance;

    tt_check(xTimerStop(hTimer, portMAX_DELAY) == pdPASS);

    return TtStatusOk;
}

uint32_t tt_timer_is_running(Timer* instance) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t)instance;

    /* Return 0: not running, 1: running */
    return (uint32_t)xTimerIsTimerActive(hTimer);
}

uint32_t tt_timer_get_expire_time(Timer* instance) {
    tt_assert(!tt_kernel_is_irq());
    tt_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t)instance;

    return (uint32_t)xTimerGetExpiryTime(hTimer);
}

void tt_timer_pending_callback(TimerPendigCallback callback, void* context, uint32_t arg) {
    BaseType_t ret = pdFAIL;
    if (tt_kernel_is_irq()) {
        ret = xTimerPendFunctionCallFromISR(callback, context, arg, NULL);
    } else {
        ret = xTimerPendFunctionCall(callback, context, arg, TtWaitForever);
    }
    tt_check(ret == pdPASS);
}

void tt_timer_set_thread_priority(TimerThreadPriority priority) {
    tt_assert(!tt_kernel_is_irq());

    TaskHandle_t task_handle = xTimerGetTimerDaemonTaskHandle();
    tt_check(task_handle); // Don't call this method before timer task start

    if (priority == TimerThreadPriorityNormal) {
        vTaskPrioritySet(task_handle, configTIMER_TASK_PRIORITY);
    } else if (priority == TimerThreadPriorityElevated) {
        vTaskPrioritySet(task_handle, configMAX_PRIORITIES - 1);
    } else {
        tt_crash();
    }
}