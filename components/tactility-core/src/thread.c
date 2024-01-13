#include "thread.h"

#include "check.h"
#include "core_defines.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kernel.h"
#include "log.h"
#include "tt_string.h"
#include <esp_log.h>

#define TAG "Thread"

#define THREAD_NOTIFY_INDEX 1 // Index 0 is used for stream buffers

// Limits
#define MAX_BITS_TASK_NOTIFY 31U
#define MAX_BITS_EVENT_GROUPS 24U

#define THREAD_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

typedef struct ThreadStdout ThreadStdout;

struct ThreadStdout {
    ThreadStdoutWriteCallback write_callback;
    TtString* buffer;
};

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
    size_t heap_size;

    ThreadStdout output;

    // Keep all non-alignable byte types in one place,
    // this ensures that the size of this structure is minimal
    bool is_static;
    bool heap_trace_enabled;

    configSTACK_DEPTH_TYPE stack_size;
};

static size_t __tt_thread_stdout_write(Thread* thread, const char* data, size_t size);
static int32_t __tt_thread_stdout_flush(Thread* thread);

/** Catch threads that are trying to exit wrong way */
__attribute__((__noreturn__)) void tt_thread_catch() { //-V1082
    // If you're here it means you're probably doing something wrong
    // with critical sections or with scheduler state
    asm volatile("nop");                // extra magic
    tt_crash("You are doing it wrong"); //-V779
    __builtin_unreachable();
}

static void tt_thread_set_state(Thread* thread, ThreadState state) {
    tt_assert(thread);
    thread->state = state;
    if (thread->state_callback) {
        thread->state_callback(state, thread->state_context);
    }
}

static void tt_thread_body(void* context) {
    tt_assert(context);
    Thread* thread = context;

    // store thread instance to thread local storage
    tt_assert(pvTaskGetThreadLocalStoragePointer(NULL, 0) == NULL);
    vTaskSetThreadLocalStoragePointer(NULL, 0, thread);

    tt_assert(thread->state == ThreadStateStarting);
    tt_thread_set_state(thread, ThreadStateRunning);

    thread->ret = thread->callback(thread->context);

    tt_assert(thread->state == ThreadStateRunning);

    if (thread->is_static) {
        ESP_LOGI(
            TAG,
            "%s service thread TCB memory will not be reclaimed",
            thread->name ? thread->name : "<unnamed service>"
        );
    }

    // flush stdout
    __tt_thread_stdout_flush(thread);

    tt_thread_set_state(thread, ThreadStateStopped);

    vTaskDelete(NULL);
    tt_thread_catch();
}

Thread* tt_thread_alloc() {
    Thread* thread = malloc(sizeof(Thread));
    // TODO: create default struct instead of using memset()
    memset(thread, 0, sizeof(Thread));
    thread->output.buffer = tt_string_alloc();
    thread->is_static = false;

    Thread* parent = NULL;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        // TLS is not available, if we called not from thread context
        parent = pvTaskGetThreadLocalStoragePointer(NULL, 0);

        if (parent && parent->appid) {
            tt_thread_set_appid(thread, parent->appid);
        } else {
            tt_thread_set_appid(thread, "unknown");
        }
    } else {
        // if scheduler is not started, we are starting driver thread
        tt_thread_set_appid(thread, "driver");
    }

    /*HalRtcHeapTrackMode mode = tt_hal_rtc_get_heap_track_mode();
    if(mode == HalRtcHeapTrackModeAll) {
        thread->heap_trace_enabled = true;
    } else if(mode == HalRtcHeapTrackModeTree && tt_thread_get_current_id()) {
        if(parent) thread->heap_trace_enabled = parent->heap_trace_enabled;
    } else */
    {
        thread->heap_trace_enabled = false;
    }

    return thread;
}

Thread* tt_thread_alloc_ex(
    const char* name,
    uint32_t stack_size,
    ThreadCallback callback,
    void* context
) {
    Thread* thread = tt_thread_alloc();
    tt_thread_set_name(thread, name);
    tt_thread_set_stack_size(thread, stack_size);
    tt_thread_set_callback(thread, callback);
    tt_thread_set_context(thread, context);
    return thread;
}

void tt_thread_free(Thread* thread) {
    tt_assert(thread);

    // Ensure that use join before free
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(thread->task_handle == NULL);

    if (thread->name) free(thread->name);
    if (thread->appid) free(thread->appid);
    tt_string_free(thread->output.buffer);

    free(thread);
}

void tt_thread_set_name(Thread* thread, const char* name) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    if (thread->name) free(thread->name);
    thread->name = name ? strdup(name) : NULL;
}

void tt_thread_set_appid(Thread* thread, const char* appid) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    if (thread->appid) free(thread->appid);
    thread->appid = appid ? strdup(appid) : NULL;
}

void tt_thread_mark_as_static(Thread* thread) {
    thread->is_static = true;
}

bool tt_thread_mark_is_service(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    assert(!TT_IS_IRQ_MODE() && (hTask != NULL));
    Thread* thread = (Thread*)pvTaskGetThreadLocalStoragePointer(hTask, 0);
    assert(thread != NULL);
    return thread->is_static;
}

void tt_thread_set_stack_size(Thread* thread, size_t stack_size) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(stack_size % 4 == 0);
    thread->stack_size = stack_size;
}

void tt_thread_set_callback(Thread* thread, ThreadCallback callback) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->callback = callback;
}

void tt_thread_set_context(Thread* thread, void* context) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->context = context;
}

void tt_thread_set_priority(Thread* thread, ThreadPriority priority) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(priority >= ThreadPriorityIdle && priority <= ThreadPriorityIsr);
    thread->priority = priority;
}

void tt_thread_set_current_priority(ThreadPriority priority) {
    UBaseType_t new_priority = priority ? priority : ThreadPriorityNormal;
    vTaskPrioritySet(NULL, new_priority);
}

ThreadPriority tt_thread_get_current_priority() {
    return (ThreadPriority)uxTaskPriorityGet(NULL);
}

void tt_thread_set_state_callback(Thread* thread, ThreadStateCallback callback) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->state_callback = callback;
}

void tt_thread_set_state_context(Thread* thread, void* context) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->state_context = context;
}

ThreadState tt_thread_get_state(Thread* thread) {
    tt_assert(thread);
    return thread->state;
}

void tt_thread_start(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->callback);
    tt_assert(thread->state == ThreadStateStopped);
    tt_assert(thread->stack_size > 0 && thread->stack_size < (UINT16_MAX * sizeof(StackType_t)));

    tt_thread_set_state(thread, ThreadStateStarting);

    uint32_t stack = thread->stack_size / sizeof(StackType_t);
    UBaseType_t priority = thread->priority ? thread->priority : ThreadPriorityNormal;
    if (thread->is_static) {
        thread->task_handle = xTaskCreateStatic(
            tt_thread_body,
            thread->name,
            stack,
            thread,
            priority,
            malloc(sizeof(StackType_t) * stack),
            malloc(sizeof(StaticTask_t))
        );
    } else {
        BaseType_t ret = xTaskCreate(
            tt_thread_body, thread->name, stack, thread, priority, &thread->task_handle
        );
        tt_check(ret == pdPASS);
    }

    tt_check(thread->task_handle);
}

void tt_thread_cleanup_tcb_event(TaskHandle_t task) {
    Thread* thread = pvTaskGetThreadLocalStoragePointer(task, 0);
    if (thread) {
        // clear thread local storage
        vTaskSetThreadLocalStoragePointer(task, 0, NULL);
        tt_assert(thread->task_handle == task);
        thread->task_handle = NULL;
    }
}

bool tt_thread_join(Thread* thread) {
    tt_assert(thread);

    tt_check(tt_thread_get_current() != thread);

    // !!! IMPORTANT NOTICE !!!
    //
    // If your thread exited, but your app stuck here: some other thread uses
    // all cpu time, which delays kernel from releasing task handle
    while (thread->task_handle) {
        tt_delay_ms(10);
    }

    return true;
}

ThreadId tt_thread_get_id(Thread* thread) {
    tt_assert(thread);
    return thread->task_handle;
}

void tt_thread_enable_heap_trace(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->heap_trace_enabled = true;
}

void tt_thread_disable_heap_trace(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    thread->heap_trace_enabled = false;
}

size_t tt_thread_get_heap_size(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->heap_trace_enabled == true);
    return thread->heap_size;
}

int32_t tt_thread_get_return_code(Thread* thread) {
    tt_assert(thread);
    tt_assert(thread->state == ThreadStateStopped);
    return thread->ret;
}

ThreadId tt_thread_get_current_id() {
    return xTaskGetCurrentTaskHandle();
}

Thread* tt_thread_get_current() {
    Thread* thread = pvTaskGetThreadLocalStoragePointer(NULL, 0);
    return thread;
}

void tt_thread_yield() {
    tt_assert(!TT_IS_IRQ_MODE());
    taskYIELD();
}

uint32_t tt_thread_flags_set(ThreadId thread_id, uint32_t flags) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    uint32_t rflags;
    BaseType_t yield;

    if ((hTask == NULL) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)) {
        rflags = (uint32_t)TtStatusErrorParameter;
    } else {
        rflags = (uint32_t)TtStatusError;

        if (TT_IS_IRQ_MODE()) {
            yield = pdFALSE;

            (void)xTaskNotifyIndexedFromISR(hTask, THREAD_NOTIFY_INDEX, flags, eSetBits, &yield);
            (void)xTaskNotifyAndQueryIndexedFromISR(
                hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags, NULL
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

uint32_t tt_thread_flags_clear(uint32_t flags) {
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

uint32_t tt_thread_flags_get(void) {
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

uint32_t tt_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout) {
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

uint32_t tt_thread_enumerate(ThreadId* thread_array, uint32_t array_items) {
    uint32_t i, count;
    TaskStatus_t* task;

    if (TT_IS_IRQ_MODE() || (thread_array == NULL) || (array_items == 0U)) {
        count = 0U;
    } else {
        vTaskSuspendAll();

        count = uxTaskGetNumberOfTasks();
        task = pvPortMalloc(count * sizeof(TaskStatus_t));

        if (task != NULL) {
            count = uxTaskGetSystemState(task, count, NULL);

            for (i = 0U; (i < count) && (i < array_items); i++) {
                thread_array[i] = (ThreadId)task[i].xHandle;
            }
            count = i;
        }
        (void)xTaskResumeAll();

        vPortFree(task);
    }

    return (count);
}

const char* tt_thread_get_name(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    const char* name;

    if (TT_IS_IRQ_MODE() || (hTask == NULL)) {
        name = NULL;
    } else {
        name = pcTaskGetName(hTask);
    }

    return (name);
}

const char* tt_thread_get_appid(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    const char* appid = "system";

    if (!TT_IS_IRQ_MODE() && (hTask != NULL)) {
        Thread* thread = (Thread*)pvTaskGetThreadLocalStoragePointer(hTask, 0);
        if (thread) {
            appid = thread->appid;
        }
    }

    return (appid);
}

uint32_t tt_thread_get_stack_space(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    uint32_t sz;

    if (TT_IS_IRQ_MODE() || (hTask == NULL)) {
        sz = 0U;
    } else {
        sz = (uint32_t)(uxTaskGetStackHighWaterMark(hTask) * sizeof(StackType_t));
    }

    return (sz);
}

static size_t __tt_thread_stdout_write(Thread* thread, const char* data, size_t size) {
    if (thread->output.write_callback != NULL) {
        thread->output.write_callback(data, size);
    } else {
        TT_LOG_I(thread->name, "%s", data);
    }
    return size;
}

static int32_t __tt_thread_stdout_flush(Thread* thread) {
    TtString* buffer = thread->output.buffer;
    size_t size = tt_string_size(buffer);
    if (size > 0) {
        __tt_thread_stdout_write(thread, tt_string_get_cstr(buffer), size);
        tt_string_reset(buffer);
    }
    return 0;
}

void tt_thread_suspend(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    vTaskSuspend(hTask);
}

void tt_thread_resume(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    if (TT_IS_IRQ_MODE()) {
        xTaskResumeFromISR(hTask);
    } else {
        vTaskResume(hTask);
    }
}

bool tt_thread_is_suspended(ThreadId thread_id) {
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    return eTaskGetState(hTask) == eSuspended;
}
