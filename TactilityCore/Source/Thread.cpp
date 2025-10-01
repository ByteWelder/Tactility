#include "Tactility/Thread.h"

#include "Tactility/Check.h"
#include "Tactility/CoreDefines.h"
#include "Tactility/EventFlag.h"
#include "Tactility/kernel/Kernel.h"
#include "Tactility/Log.h"
#include "Tactility/TactilityCoreConfig.h"

#include <string>

namespace tt {

#define TAG "Thread"

#define THREAD_NOTIFY_INDEX 1 // Index 0 is used for stream buffers

// Limits
#define MAX_BITS_TASK_NOTIFY 31U
#define MAX_BITS_EVENT_GROUPS 24U

#define THREAD_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

static_assert(static_cast<UBaseType_t>(Thread::Priority::Critical) <= TT_CONFIG_THREAD_MAX_PRIORITIES, "highest thread priority is higher than max priority");
static_assert(TT_CONFIG_THREAD_MAX_PRIORITIES <= configMAX_PRIORITIES, "highest tactility priority is higher than max FreeRTOS priority");

void Thread::setState(Thread::State newState) {
    state = newState;
    if (stateCallback) {
        stateCallback(state, stateCallbackContext);
    }
}

static_assert(configSUPPORT_DYNAMIC_ALLOCATION == 1);

/** Catch threads that are trying to exit wrong way */
__attribute__((__noreturn__)) void threadCatch() { //-V1082
    // If you're here it means you're probably doing something wrong with critical sections or with scheduler state
    asm volatile("nop");
    tt_crash();
    __builtin_unreachable();
}

void Thread::mainBody(void* context) {
    assert(context != nullptr);
    auto* thread = static_cast<Thread*>(context);

    // Store thread data instance to thread local storage
    assert(pvTaskGetThreadLocalStoragePointer(nullptr, 0) == nullptr);
    vTaskSetThreadLocalStoragePointer(nullptr, 0, thread);

    TT_LOG_I(TAG, "Starting %s", thread->name.c_str());
    assert(thread->state == Thread::State::Starting);
    thread->setState(Thread::State::Running);
    thread->callbackResult = thread->mainFunction();
    assert(thread->state == Thread::State::Running);

    thread->setState(Thread::State::Stopped);
    TT_LOG_I(TAG, "Stopped %s", thread->name.c_str());

    vTaskSetThreadLocalStoragePointer(nullptr, 0, nullptr);
    thread->taskHandle = nullptr;

    vTaskDelete(nullptr);
    threadCatch();
}

Thread::Thread(
    std::string name,
    configSTACK_DEPTH_TYPE stackSize,
    MainFunction function,
    portBASE_TYPE affinity
) :
    mainFunction(function),
    name(std::move(name)),
    stackSize(stackSize),
    affinity(affinity)
{}

Thread::~Thread() {
    // Ensure that use join before free
    assert(state == State::Stopped);
    assert(taskHandle == nullptr);
}

void Thread::setName(std::string newName) {
    assert(state == State::Stopped);
    name = std::move(newName);
}

void Thread::setStackSize(size_t newStackSize) {
    assert(state == State::Stopped);
    assert(stackSize % 4 == 0);
    stackSize = newStackSize;
}

void Thread::setAffinity(portBASE_TYPE newAffinity) {
    assert(state == State::Stopped);
    affinity = newAffinity;
}

void Thread::setCallback(Callback callback, _Nullable void* callbackContext) {
    assert(state == State::Stopped);
    mainFunction = [callback, callbackContext] {
        return callback(callbackContext);
    };
}

void Thread::setMainFunction(MainFunction function) {
    assert(state == State::Stopped);
    mainFunction = function;
}

void Thread::setPriority(Priority newPriority) {
    assert(state == State::Stopped);
    priority = newPriority;
}


void Thread::setStateCallback(StateCallback callback, _Nullable void* callbackContext) {
    assert(state == State::Stopped);
    stateCallback = callback;
    stateCallbackContext = callbackContext;
}

Thread::State Thread::getState() const {
    return state;
}

void Thread::start() {
    assert(mainFunction);
    assert(state == State::Stopped);
    assert(stackSize > 0U && stackSize < (UINT16_MAX * sizeof(StackType_t)));

    setState(State::Starting);

    uint32_t stack_depth = stackSize / sizeof(StackType_t);

    BaseType_t result;
    if (affinity != -1) {
#ifdef ESP_PLATFORM
        result = xTaskCreatePinnedToCore(
            mainBody,
            name.c_str(),
            stack_depth,
            this,
            static_cast<UBaseType_t>(priority),
            &taskHandle,
            affinity
        );
#else
        TT_LOG_W(TAG, "Pinned tasks are not supported by current FreeRTOS platform - creating regular one");
        result = xTaskCreate(
            mainBody,
            name.c_str(),
            stack_depth,
            this,
            static_cast<UBaseType_t>(priority),
            &taskHandle
        );
#endif
    } else {
        result = xTaskCreate(
            mainBody,
            name.c_str(),
            stack_depth,
            this,
            static_cast<UBaseType_t>(priority),
            &taskHandle
        );
    }

    tt_check(result == pdPASS);
    tt_check(state == State::Stopped || taskHandle);
}

bool Thread::join(TickType_t timeout, TickType_t pollInterval) {
    tt_check(getCurrent() != this);

    // !!! IMPORTANT NOTICE !!!
    //
    // If your thread exited, but your app stuck here: some other thread uses
    // all cpu time, which delays kernel from releasing task handle
    TickType_t start_ticks = kernel::getTicks();
    while (taskHandle) {
        kernel::delayTicks(pollInterval);
        if ((kernel::getTicks() - start_ticks) > timeout) {
            return false;
        }
    }

    return true;
}

ThreadId Thread::getId() const {
    return taskHandle;
}

int32_t Thread::getReturnCode() const {
    assert(state == State::Stopped);
    return callbackResult;
}

Thread* Thread::getCurrent() {
    return static_cast<Thread*>(pvTaskGetThreadLocalStoragePointer(nullptr, 0));
}

uint32_t Thread::setFlags(ThreadId threadId, uint32_t flags) {
    auto hTask = (TaskHandle_t)threadId;
    uint32_t rflags;
    BaseType_t yield;

    if ((hTask == nullptr) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)) {
        rflags = (uint32_t)EventFlag::ErrorParameter;
    } else {
        rflags = (uint32_t)EventFlag::Error;

        if (kernel::isIsr()) {
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

uint32_t Thread::clearFlags(uint32_t flags) {
    TaskHandle_t hTask;
    uint32_t rflags, cflags;

    if (kernel::isIsr()) {
        rflags = (uint32_t)EventFlag::ErrorISR;
    } else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U) {
        rflags = (uint32_t)EventFlag::ErrorParameter;
    } else {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &cflags) ==
            pdPASS) {
            rflags = cflags;
            cflags &= ~flags;

            if (xTaskNotifyIndexed(hTask, THREAD_NOTIFY_INDEX, cflags, eSetValueWithOverwrite) !=
                pdPASS) {
                rflags = (uint32_t)EventFlag::Error;
            }
        } else {
            rflags = (uint32_t)EventFlag::Error;
        }
    }

    /* Return flags before clearing */
    return (rflags);
}

uint32_t Thread::getFlags() {
    TaskHandle_t hTask;
    uint32_t rflags;

    if (kernel::isIsr()) {
        rflags = (uint32_t)EventFlag::ErrorISR;
    } else {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags) !=
            pdPASS) {
            rflags = (uint32_t)EventFlag::Error;
        }
    }

    return (rflags);
}

uint32_t Thread::awaitFlags(uint32_t flags, uint32_t options, uint32_t timeout) {
    uint32_t rflags, nval;
    uint32_t clear;
    TickType_t t0, td, tout;
    BaseType_t rval;

    if (kernel::isIsr()) {
        rflags = (uint32_t)EventFlag::ErrorISR;
    } else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U) {
        rflags = (uint32_t)EventFlag::ErrorParameter;
    } else {
        if ((options & EventFlag::NoClear) == EventFlag::NoClear) {
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

                if ((options & EventFlag::WaitAll) == EventFlag::WaitAll) {
                    if ((flags & rflags) == flags) {
                        break;
                    } else {
                        if (timeout == 0U) {
                            rflags = (uint32_t)EventFlag::ErrorResource;
                            break;
                        }
                    }
                } else {
                    if ((flags & rflags) != 0) {
                        break;
                    } else {
                        if (timeout == 0U) {
                            rflags = (uint32_t)EventFlag::ErrorResource;
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
                    rflags = (uint32_t)EventFlag::ErrorResource;
                } else {
                    rflags = (uint32_t)EventFlag::ErrorTimeout;
                }
            }
        } while (rval != pdFAIL);
    }

    /* Return flags before clearing */
    return (rflags);
}

uint32_t Thread::getStackSpace(ThreadId threadId) {
    auto hTask = (TaskHandle_t)threadId;
    uint32_t sz;

    if (kernel::isIsr() || (hTask == nullptr)) {
        sz = 0U;
    } else {
        sz = (uint32_t)(uxTaskGetStackHighWaterMark(hTask) * sizeof(StackType_t));
    }

    return (sz);
}

void Thread::suspend(ThreadId threadId) {
    auto hTask = (TaskHandle_t)threadId;
    vTaskSuspend(hTask);
}

void Thread::resume(ThreadId threadId) {
    auto hTask = (TaskHandle_t)threadId;
    if (kernel::isIsr()) {
        xTaskResumeFromISR(hTask);
    } else {
        vTaskResume(hTask);
    }
}

bool Thread::isSuspended(ThreadId threadId) {
    auto hTask = (TaskHandle_t)threadId;
    return eTaskGetState(hTask) == eSuspended;
}

} // namespace
