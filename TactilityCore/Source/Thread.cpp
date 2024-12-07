#include "Thread.h"
#include <string>

#include "Check.h"
#include "CoreDefines.h"
#include "kernel/Kernel.h"
#include "Log.h"

namespace tt {

#define TAG "Thread"

#define THREAD_NOTIFY_INDEX 1 // Index 0 is used for stream buffers

// Limits
#define MAX_BITS_TASK_NOTIFY 31U
#define MAX_BITS_EVENT_GROUPS 24U

#define THREAD_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

static_assert(Thread::PriorityHighest <= TT_CONFIG_THREAD_MAX_PRIORITIES, "highest thread priority is higher than max priority");
static_assert(TT_CONFIG_THREAD_MAX_PRIORITIES <= configMAX_PRIORITIES, "highest tactility priority is higher than max FreeRTOS priority");

void setState(Thread::Data* data, Thread::State state) {
    data->state = state;
    if (data->stateCallback) {
        data->stateCallback(state, data->stateCallbackContext);
    }
}

/** Catch threads that are trying to exit wrong way */
__attribute__((__noreturn__)) void thread_catch() { //-V1082
    // If you're here it means you're probably doing something wrong
    // with critical sections or with scheduler state
    asm volatile("nop");                // extra magic
    tt_crash("You are doing it wrong"); //-V779
    __builtin_unreachable();
}


static void thread_body(void* context) {
    tt_assert(context);
    auto* data = static_cast<Thread::Data*>(context);

    // Store thread data instance to thread local storage
    tt_assert(pvTaskGetThreadLocalStoragePointer(nullptr, 0) == nullptr);
    vTaskSetThreadLocalStoragePointer(nullptr, 0, data->thread);

    tt_assert(data->state == Thread::StateStarting);
    setState(data, Thread::StateRunning);
    data->callbackResult = data->callback(data->callbackContext);
    tt_assert(data->state == Thread::StateRunning);

    if (data->isStatic) {
        TT_LOG_I(
            TAG,
            "%s static task memory will not be reclaimed",
            data->name.empty() ? "<unnamed service>" : data->name.c_str()
        );
    }

    setState(data, Thread::StateStopped);

    vTaskSetThreadLocalStoragePointer(nullptr, 0, nullptr);
    data->taskHandle = nullptr;

    vTaskDelete(nullptr);
    thread_catch();
}

Thread::Thread() :
    data({
        .taskHandle = nullptr,
        .state = StateStopped,
        .callback = nullptr,
        .callbackContext = nullptr,
        .callbackResult = 0,
        .stateCallback = nullptr,
        .stateCallbackContext = nullptr,
        .name = std::string(),
        .priority = PriorityNormal,
        .isStatic = false,
        .stackSize = 0,
    }) { }

Thread::Thread(
    const std::string& name,
    configSTACK_DEPTH_TYPE stackSize,
    Callback callback,
    _Nullable void* callbackContext) :
    data({
        .taskHandle = nullptr,
        .state = StateStopped,
        .callback = callback,
        .callbackContext = callbackContext,
        .callbackResult = 0,
        .stateCallback = nullptr,
        .stateCallbackContext = nullptr,
        .name = name,
        .priority = PriorityNormal,
        .isStatic = false,
        .stackSize = stackSize,
    }) { }

Thread::~Thread() {
    // Ensure that use join before free
    tt_assert(data.state == StateStopped);
    tt_assert(data.taskHandle == nullptr);
}

void Thread::setName(const std::string& newName) {
    tt_assert(data.state == StateStopped);
    data.name = newName;
}


void Thread::markAsStatic() {
    data.isStatic = true;
}

bool Thread::isMarkedAsStatic() const {
    return data.isStatic;
}

void Thread::setStackSize(size_t stackSize) {
    tt_assert(data.state == StateStopped);
    tt_assert(stackSize % 4 == 0);
    data.stackSize = stackSize;
}

void Thread::setCallback(Callback callback, _Nullable void* callbackContext) {
    tt_assert(data.state == StateStopped);
    data.callback = callback;
    data.callbackContext = callbackContext;
}


void Thread::setPriority(Priority priority) {
    tt_assert(data.state == StateStopped);
    tt_assert(priority >= 0 && priority <= TT_CONFIG_THREAD_MAX_PRIORITIES);
    data.priority = priority;
}


void Thread::setStateCallback(StateCallback callback, _Nullable void* callbackContext) {
    tt_assert(data.state == StateStopped);
    data.stateCallback = callback;
    data.stateCallbackContext = callbackContext;
}

Thread::State Thread::getState() const {
    return data.state;
}

void Thread::start() {
    tt_assert(data.callback);
    tt_assert(data.state == StateStopped);
    tt_assert(data.stackSize > 0 && data.stackSize < (UINT16_MAX * sizeof(StackType_t)));

    setState(&data, StateStarting);

    uint32_t stack_depth = data.stackSize / sizeof(StackType_t);
    if (data.isStatic) {
#if configSUPPORT_STATIC_ALLOCATION == 1
        data.taskHandle = xTaskCreateStatic(
            thread_body,
            data.name.c_str(),
            stack_depth,
            &data,
            data.priority,
            static_cast<StackType_t*>(malloc(sizeof(StackType_t) * stack_depth)),
            static_cast<StaticTask_t*>(malloc(sizeof(StaticTask_t)))
        );
#else
        TT_LOG_E(TAG, "static tasks are not supported by current FreeRTOS config/platform - creating regular one");
        BaseType_t result = xTaskCreate(
            thread_body, data.name.c_str(), stack_depth, this, data.priority, &(data.taskHandle)
        );
        tt_check(result == pdPASS);
#endif
    } else {
        BaseType_t result = xTaskCreate(
            thread_body, data.name.c_str(), stack_depth, this, data.priority, &(data.taskHandle)
        );
        tt_check(result == pdPASS);
    }

    tt_check(data.state == StateStopped || data.taskHandle);
}

bool Thread::join() {
    tt_check(thread_get_current() != this);

    // !!! IMPORTANT NOTICE !!!
    //
    // If your thread exited, but your app stuck here: some other thread uses
    // all cpu time, which delays kernel from releasing task handle
    while (data.taskHandle) {
        kernel::delayMillis(10);
    }

    return true;
}

ThreadId Thread::getId() {
    return data.taskHandle;
}

int32_t Thread::getReturnCode() {
    tt_assert(data.state == StateStopped);
    return data.callbackResult;
}

ThreadId thread_get_current_id() {
    return xTaskGetCurrentTaskHandle();
}

Thread* thread_get_current() {
    auto* thread = static_cast<Thread*>(pvTaskGetThreadLocalStoragePointer(nullptr, 0));
    return thread;
}

void thread_set_current_priority(Thread::Priority priority) {
    UBaseType_t new_priority = priority ? priority : Thread::PriorityNormal;
    vTaskPrioritySet(nullptr, new_priority);
}

Thread::Priority thread_get_current_priority() {
    return (Thread::Priority)uxTaskPriorityGet(nullptr);
}

void thread_yield() {
    tt_assert(!TT_IS_IRQ_MODE());
    taskYIELD();
}

bool thread_mark_is_static(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    assert(!TT_IS_IRQ_MODE() && (hTask != nullptr));
    auto* thread = (Thread*)pvTaskGetThreadLocalStoragePointer(hTask, 0);
    assert(thread != nullptr);
    return thread->isMarkedAsStatic();
}

uint32_t thread_flags_set(ThreadId thread_id, uint32_t flags) {
    auto hTask = (TaskHandle_t)thread_id;
    uint32_t rflags;
    BaseType_t yield;

    if ((hTask == nullptr) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)) {
        rflags = (uint32_t)TtStatusErrorParameter;
    } else {
        rflags = (uint32_t)TtStatusError;

        if (TT_IS_IRQ_MODE()) {
            yield = pdFALSE;

            (void)xTaskNotifyIndexedFromISR(hTask, THREAD_NOTIFY_INDEX, flags, eSetBits, &yield);
            (void)xTaskNotifyAndQueryIndexedFromISR(
                hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags, nullptr
            );

            portYIELD_FROM_ISR(yield);
        } else {
            (void)xTaskNotifyIndexed(hTask, THREAD_NOTIFY_INDEX, flags, eSetBits);
            (void)xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags);
        }
    }
    /* Return flags after setting */
    return (rflags);
}

uint32_t thread_flags_clear(uint32_t flags) {
    TaskHandle_t hTask;
    uint32_t rflags, cflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = (uint32_t)TtStatusErrorISR;
    } else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U) {
        rflags = (uint32_t)TtStatusErrorParameter;
    } else {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &cflags) ==
            pdPASS) {
            rflags = cflags;
            cflags &= ~flags;

            if (xTaskNotifyIndexed(hTask, THREAD_NOTIFY_INDEX, cflags, eSetValueWithOverwrite) !=
                pdPASS) {
                rflags = (uint32_t)TtStatusError;
            }
        } else {
            rflags = (uint32_t)TtStatusError;
        }
    }

    /* Return flags before clearing */
    return (rflags);
}

uint32_t thread_flags_get() {
    TaskHandle_t hTask;
    uint32_t rflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = (uint32_t)TtStatusErrorISR;
    } else {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags) !=
            pdPASS) {
            rflags = (uint32_t)TtStatusError;
        }
    }

    return (rflags);
}

uint32_t thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout) {
    uint32_t rflags, nval;
    uint32_t clear;
    TickType_t t0, td, tout;
    BaseType_t rval;

    if (TT_IS_IRQ_MODE()) {
        rflags = (uint32_t)TtStatusErrorISR;
    } else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U) {
        rflags = (uint32_t)TtStatusErrorParameter;
    } else {
        if ((options & TtFlagNoClear) == TtFlagNoClear) {
            clear = 0U;
        } else {
            clear = flags;
        }

        rflags = 0U;
        tout = timeout;

        t0 = xTaskGetTickCount();
        do {
            rval = xTaskNotifyWaitIndexed(THREAD_NOTIFY_INDEX, 0, clear, &nval, tout);

            if (rval == pdPASS) {
                rflags &= flags;
                rflags |= nval;

                if ((options & TtFlagWaitAll) == TtFlagWaitAll) {
                    if ((flags & rflags) == flags) {
                        break;
                    } else {
                        if (timeout == 0U) {
                            rflags = (uint32_t)TtStatusErrorResource;
                            break;
                        }
                    }
                } else {
                    if ((flags & rflags) != 0) {
                        break;
                    } else {
                        if (timeout == 0U) {
                            rflags = (uint32_t)TtStatusErrorResource;
                            break;
                        }
                    }
                }

                /* Update timeout */
                td = xTaskGetTickCount() - t0;

                if (td > tout) {
                    tout = 0;
                } else {
                    tout -= td;
                }
            } else {
                if (timeout == 0) {
                    rflags = (uint32_t)TtStatusErrorResource;
                } else {
                    rflags = (uint32_t)TtStatusErrorTimeout;
                }
            }
        } while (rval != pdFAIL);
    }

    /* Return flags before clearing */
    return (rflags);
}

const char* thread_get_name(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    const char* name;

    if (TT_IS_IRQ_MODE() || (hTask == nullptr)) {
        name = nullptr;
    } else {
        name = pcTaskGetName(hTask);
    }

    return (name);
}

uint32_t thread_get_stack_space(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    uint32_t sz;

    if (TT_IS_IRQ_MODE() || (hTask == nullptr)) {
        sz = 0U;
    } else {
        sz = (uint32_t)(uxTaskGetStackHighWaterMark(hTask) * sizeof(StackType_t));
    }

    return (sz);
}

void thread_suspend(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    vTaskSuspend(hTask);
}

void thread_resume(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    if (TT_IS_IRQ_MODE()) {
        xTaskResumeFromISR(hTask);
    } else {
        vTaskResume(hTask);
    }
}

bool thread_is_suspended(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    return eTaskGetState(hTask) == eSuspended;
}

} // namespace
