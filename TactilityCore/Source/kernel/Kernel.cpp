#include "kernel/Kernel.h"
#include "Check.h"
#include "CoreDefines.h"
#include "CoreTypes.h"
#include "RtosCompatTask.h"

#ifdef ESP_PLATFORM
#include "rom/ets_sys.h"
#else
#include <unistd.h>
#endif

namespace tt::kernel {

bool isRunning() {
    return xTaskGetSchedulerState() != taskSCHEDULER_RUNNING;
}

int32_t lock() {
    assert(!TT_IS_ISR());

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

int32_t unlock() {
    assert(!TT_IS_ISR());

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

int32_t restoreLock(int32_t lock) {
    assert(!TT_IS_ISR());

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

uint32_t getTickFrequency() {
    /* Return frequency in hertz */
    return (configTICK_RATE_HZ);
}

void delayTicks(TickType_t ticks) {
    assert(!TT_IS_ISR());
    if (ticks == 0U) {
        taskYIELD();
    } else {
        vTaskDelay(ticks);
    }
}

TtStatus delayUntilTick(TickType_t tick) {
    assert(!TT_IS_ISR());

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

TickType_t getTicks() {
    TickType_t ticks;

    if (TT_IS_ISR() != 0U) {
        ticks = xTaskGetTickCountFromISR();
    } else {
        ticks = xTaskGetTickCount();
    }

    return ticks;
}

TickType_t millisToTicks(uint32_t milliseconds) {
#if configTICK_RATE_HZ == 1000
    return (TickType_t)milliseconds;
#else
    return (TickType_t)((float)configTICK_RATE_HZ) / 1000.0f * (float)milliseconds;
#endif
}

void delayMillis(uint32_t milliseconds) {
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (milliseconds > 0 && milliseconds < portMAX_DELAY - 1) {
            milliseconds += 1;
        }
#if configTICK_RATE_HZ_RAW == 1000
        tt_delay_tick(milliseconds);
#else
        delayTicks(kernel::millisToTicks(milliseconds));
#endif
    } else if (milliseconds > 0) {
        kernel::delayMicros(milliseconds * 1000);
    }
}

void delayMicros(uint32_t microseconds) {
#ifdef ESP_PLATFORM
    ets_delay_us(microseconds);
#else
    usleep(microseconds);
#endif
}

Platform getPlatform() {
#ifdef ESP_PLATFORM
    return PlatformEsp;
#else
    return PlatformSimulator;
#endif
}

} // namespace
