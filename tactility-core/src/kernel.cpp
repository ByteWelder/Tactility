#include "kernel.h"
#include "check.h"
#include "core_defines.h"
#include "core_types.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef ESP_PLATFORM
#include "rom/ets_sys.h"
#else
#include <unistd.h>
#endif

namespace tt {

bool kernel_is_irq() {
    return TT_IS_IRQ_MODE();
}

bool kernel_is_running() {
    return xTaskGetSchedulerState() != taskSCHEDULER_RUNNING;
}

int32_t kernel_lock() {
    tt_assert(!kernel_is_irq());

    int32_t lock;

    switch (xTaskGetSchedulerState()) {
        case taskSCHEDULER_SUSPENDED:
            lock = 1;
            break;

        case taskSCHEDULER_RUNNING:
            vTaskSuspendAll();
            lock = 0;
            break;

        case taskSCHEDULER_NOT_STARTED:
        default:
            lock = (int32_t)TtStatusError;
            break;
    }

    /* Return previous lock state */
    return (lock);
}

int32_t kernel_unlock() {
    tt_assert(!kernel_is_irq());

    int32_t lock;

    switch (xTaskGetSchedulerState()) {
        case taskSCHEDULER_SUSPENDED:
            lock = 1;

            if (xTaskResumeAll() != pdTRUE) {
                if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) {
                    lock = (int32_t)TtStatusError;
                }
            }
            break;

        case taskSCHEDULER_RUNNING:
            lock = 0;
            break;

        case taskSCHEDULER_NOT_STARTED:
        default:
            lock = (int32_t)TtStatusError;
            break;
    }

    /* Return previous lock state */
    return (lock);
}

int32_t kernel_restore_lock(int32_t lock) {
    tt_assert(!kernel_is_irq());

    switch (xTaskGetSchedulerState()) {
        case taskSCHEDULER_SUSPENDED:
        case taskSCHEDULER_RUNNING:
            if (lock == 1) {
                vTaskSuspendAll();
            } else {
                if (lock != 0) {
                    lock = (int32_t)TtStatusError;
                } else {
                    if (xTaskResumeAll() != pdTRUE) {
                        if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
                            lock = (int32_t)TtStatusError;
                        }
                    }
                }
            }
            break;

        case taskSCHEDULER_NOT_STARTED:
        default:
            lock = (int32_t)TtStatusError;
            break;
    }

    /* Return new lock state */
    return (lock);
}

uint32_t kernel_get_tick_frequency() {
    /* Return frequency in hertz */
    return (configTICK_RATE_HZ);
}

void delay_tick(uint32_t ticks) {
    tt_assert(!kernel_is_irq());
    if (ticks == 0U) {
        taskYIELD();
    } else {
        vTaskDelay(ticks);
    }
}

TtStatus delay_until_tick(uint32_t tick) {
    tt_assert(!kernel_is_irq());

    TickType_t tcnt, delay;
    TtStatus stat;

    stat = TtStatusOk;
    tcnt = xTaskGetTickCount();

    /* Determine remaining number of tick to delay */
    delay = (TickType_t)tick - tcnt;

    /* Check if target tick has not expired */
    if ((delay != 0U) && (0 == (delay >> (8 * sizeof(TickType_t) - 1)))) {
        if (xTaskDelayUntil(&tcnt, delay) == pdFALSE) {
            /* Did not delay */
            stat = TtStatusError;
        }
    } else {
        /* No delay or already expired */
        stat = TtStatusErrorParameter;
    }

    /* Return execution status */
    return (stat);
}

uint32_t get_tick() {
    TickType_t ticks;

    if (kernel_is_irq() != 0U) {
        ticks = xTaskGetTickCountFromISR();
    } else {
        ticks = xTaskGetTickCount();
    }

    return ticks;
}

uint32_t ms_to_ticks(uint32_t milliseconds) {
#if configTICK_RATE_HZ == 1000
    return milliseconds;
#else
    return (uint32_t)((float)configTICK_RATE_HZ) / 1000.0f * (float)milliseconds;
#endif
}

void delay_ms(uint32_t milliseconds) {
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (milliseconds > 0 && milliseconds < portMAX_DELAY - 1) {
            milliseconds += 1;
        }
#if configTICK_RATE_HZ_RAW == 1000
        tt_delay_tick(milliseconds);
#else
        delay_tick(ms_to_ticks(milliseconds));
#endif
    } else if (milliseconds > 0) {
        delay_us(milliseconds * 1000);
    }
}

void delay_us(uint32_t microseconds) {
#ifdef ESP_PLATFORM
    ets_delay_us(microseconds);
#else
    usleep(microseconds);
#endif
}

Platform get_platform() {
#ifdef ESP_PLATFORM
    return PlatformEsp;
#else
    return PlatformPc;
#endif
}

} // namespace
