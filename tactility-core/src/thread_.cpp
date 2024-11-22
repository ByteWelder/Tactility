#include "thread.h"
#include <cstdlib>
#include <cstring>

#include "Check.h"
#include "CoreDefines.h"
#include "kernel.h"
#include "log.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

namespace tt {

#define TAG "Thread"

#define THREAD_NOTIFY_INDEX 1 // Index 0 is used for stream buffers

// Limits
#define MAX_BITS_TASK_NOTIFY 31U
#define MAX_BITS_EVENT_GROUPS 24U

#define THREAD_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

static_assert(ThreadPriorityHighest <= TT_CONFIG_THREAD_MAX_PRIORITIES, "highest thread priority is higher than max priority");
static_assert(TT_CONFIG_THREAD_MAX_PRIORITIES <= configMAX_PRIORITIES, "highest tactility priority is higher than max FreeRTOS priority");

struct Thread {
    ThreadState state;
    int32_t ret;

    ThreadCallback callback;
    void* context;

    ThreadStateCallback state_callback;
    void* state_context;

    char* name;
    char* appid;

    ThreadPriority priority;

    TaskHandle_t task_handle;

    // Keep all non-alignable byte types in one place,
    // this ensures that the size of this structure is minimal
    bool is_static;

    configSTACK_DEPTH_TYPE stack_size;
};

/** Catch threads that are trying to exit wrong way */
__attribute__((__noreturn__)) void thread_catch() { //-V1082
    // If you're here it means you're probably doing something wrong
    // with critical sections or with scheduler state
    asm volatile("nop");                // extra magic
    tt_crash("You are doing it wrong"); //-V779
    __builtin_unreachable();
}

static void thread_set_state(Thread* thread, ThreadState state) {
    tt_assert(thread);
    thread->state = state;
    if (thread->state_callback) {
        thread->state_callback(state, thread->state_context);
    }
}

static void thread_body(void* context) {
    tt_assert(context);
    auto* thread = static_cast<Thread*>(context);

    // Store thread instance to thread local storage
    tt_assert(pvTaskGetThreadLocalStoragePointer(nullptr, 0) == nullptr);
    vTaskSetThreadLocalStoragePointer(nullptr, 0, thread);

    tt_assert(thread->state == ThreadStateStarting);
    thread_set_state(thread, ThreadStateRunning);

    thread->ret = thread->callback(thread->context);

    tt_assert(thread->state == ThreadStateRunning);

    if (thread->is_static) {
        TT_LOG_I(
            TAG,
            "%s static task memory will not be reclaimed",
            thread->name ? thread->name : "<unnamed service>"
        );
    }

    thread_set_state(thread, ThreadStateStopped);

    vTaskSetThreadLocalStoragePointer(nullptr, 0, nullptr);
    thread->task_handle = nullptr;

    vTaskDelete(nullptr);
    thread_catch();
}

Thread* thread_alloc() {
    auto* thread = static_cast<Thread*>(malloc(sizeof(Thread)));
    // TODO: create default struct instead of using memset()
    memset(thread, 0, sizeof(Thread));
    thread->is_static = false;

    Thread* parent = nullptr;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        // TLS is not available, if we called not from thread context
        parent = (Thread*)pvTaskGetThreadLocalStoragePointer(nullptr, 0);

        if (parent && parent->appid) {
            thread_set_appid(thread, parent->appid);
        } else {
            thread_set_appid(thread, "unknown");
        }
    } else {
        // If scheduler is not started, we are starting driver thread
        thread_set_appid(thread, "driver");
    }

    return thread;
}

Thread* thread_alloc_ex(
    const char* name,
    uint32_t stack_size,
    ThreadCallback callback,
    void* context
) {
    Thread* thread = thread_alloc();
    thread_set_name(thread, name);
    thread_set_stack_size(thread, stack_size);
    thread_set_callback(thread, callback);
    thread_set_context(thread, context);
    return thread;
}

void thread_free(Thread* thread) {
    tt_assert(thread);

    // Ensure that use join before free
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(thread->task_handle == nullptr);

    if (thread->name) free(thread->name);
    if (thread->appid) free(thread->appid);

    free(thread);
}

void thread_set_name(Thread* thread, const char* name) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    if (thread->name) free(thread->name);
    thread->name = name ? strdup(name) : nullptr;
}

void thread_set_appid(Thread* thread, const char* appid) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    if (thread->appid) free(thread->appid);
    thread->appid = appid ? strdup(appid) : nullptr;
}

void thread_mark_as_static(Thread* thread) {
    thread->is_static = true;
}

bool thread_mark_is_static(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    assert(!TT_IS_IRQ_MODE() && (hTask != nullptr));
    auto* thread = (Thread*)pvTaskGetThreadLocalStoragePointer(hTask, 0);
    assert(thread != nullptr);
    return thread->is_static;
}

void thread_set_stack_size(Thread* thread, size_t stack_size) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(stack_size % 4 == 0);
    thread->stack_size = stack_size;
}

void thread_set_callback(Thread* thread, ThreadCallback callback) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->callback = callback;
}

void thread_set_context(Thread* thread, void* context) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->context = context;
}

void thread_set_priority(Thread* thread, ThreadPriority priority) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(priority >= 0 && priority <= TT_CONFIG_THREAD_MAX_PRIORITIES);
    thread->priority = priority;
}

void thread_set_current_priority(ThreadPriority priority) {
    UBaseType_t new_priority = priority ? priority : ThreadPriorityNormal;
    vTaskPrioritySet(nullptr, new_priority);
}

ThreadPriority thread_get_current_priority() {
    return (ThreadPriority)uxTaskPriorityGet(nullptr);
}

void thread_set_state_callback(Thread* thread, ThreadStateCallback callback) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->state_callback = callback;
}

void thread_set_state_context(Thread* thread, void* context) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->state_context = context;
}

ThreadState thread_get_state(Thread* thread) {
    tt_assert(thread);
    return thread->state;
}

void thread_start(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->callback);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(thread->stack_size > 0 && thread->stack_size < (UINT16_MAX * sizeof(StackType_t)));

    thread_set_state(thread, ThreadStateStarting);

    uint32_t stack = thread->stack_size / sizeof(StackType_t);
    UBaseType_t priority = thread->priority ? thread->priority : ThreadPriorityNormal;
    if (thread->is_static) {
#if configSUPPORT_STATIC_ALLOCATION == 1
        thread->task_handle = xTaskCreateStatic(
            thread_body,
            thread->name,
            stack,
            thread,
            priority,
            static_cast<StackType_t*>(malloc(sizeof(StackType_t) * stack)),
            static_cast<StaticTask_t*>(malloc(sizeof(StaticTask_t)))
        );
#else
        TT_LOG_E(TAG, "static tasks are not supported by current FreeRTOS config/platform - creating regular one");
        BaseType_t ret = xTaskCreate(
            thread_body, thread->name, stack, thread, priority, &(thread->task_handle)
        );
        tt_check(ret == pdPASS);
#endif
    } else {
        BaseType_t ret = xTaskCreate(
            thread_body, thread->name, stack, thread, priority, &(thread->task_handle)
        );
        tt_check(ret == pdPASS);
    }

    tt_check(thread->state == ThreadStateStopped || thread->task_handle);
}

bool thread_join(Thread* thread) {
    tt_assert(thread);

    tt_check(thread_get_current() != thread);

    // !!! IMPORTANT NOTICE !!!
    //
    // If your thread exited, but your app stuck here: some other thread uses
    // all cpu time, which delays kernel from releasing task handle
    while (thread->task_handle) {
        delay_ms(10);
    }

    return true;
}

ThreadId thread_get_id(Thread* thread) {
    tt_assert(thread);
    return thread->task_handle;
}

int32_t thread_get_return_code(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    return thread->ret;
}

ThreadId thread_get_current_id() {
    return xTaskGetCurrentTaskHandle();
}

Thread* thread_get_current() {
    auto* thread = static_cast<Thread*>(pvTaskGetThreadLocalStoragePointer(nullptr, 0));
    return thread;
}

void thread_yield() {
    tt_assert(!TT_IS_IRQ_MODE());
    taskYIELD();
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

const char* thread_get_appid(ThreadId thread_id) {
    auto hTask = (TaskHandle_t)thread_id;
    const char* appid = "system";

    if (!TT_IS_IRQ_MODE() && (hTask != nullptr)) {
        auto* thread = (Thread*)pvTaskGetThreadLocalStoragePointer(hTask, 0);
        if (thread) {
            appid = thread->appid;
        }
    }

    return (appid);
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
