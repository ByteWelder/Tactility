#include "kernel/Kernel.h"
#include "CoreDefines.h"
#include "RtosCompatTask.h"

#ifdef ESP_PLATFORM
#include "rom/ets_sys.h"
#else
#include <cassert>
#include <unistd.h>
#endif

namespace tt::kernel {

bool isRunning() {
    return xTaskGetSchedulerState() != taskSCHEDULER_RUNNING;
}

bool lock() {
    assert(!TT_IS_ISR());

    int32_t lock;

    switch (xTaskGetSchedulerState()) {
        // Already suspended
        case taskSCHEDULER_SUSPENDED:
            return true;

        case taskSCHEDULER_RUNNING:
            vTaskSuspendAll();
            return true;

        case taskSCHEDULER_NOT_STARTED:
        default:
            return false;
    }

    /* Return previous lock state */
    return (lock);
}

bool unlock() {
    assert(!TT_IS_ISR());

    switch (xTaskGetSchedulerState()) {
        case taskSCHEDULER_SUSPENDED:
            if (xTaskResumeAll() != pdTRUE) {
                if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) {
                    return false;
                }
            }
            return true;

        case taskSCHEDULER_RUNNING:
            return true;

        case taskSCHEDULER_NOT_STARTED:
        default:
            return false;
    }
}

bool restoreLock(bool lock) {
    assert(!TT_IS_ISR());

    switch (xTaskGetSchedulerState()) {
        case taskSCHEDULER_SUSPENDED:
        case taskSCHEDULER_RUNNING:
            if (lock) {
                vTaskSuspendAll();
                return true;
            } else {
                if (xTaskResumeAll() != pdTRUE) {
                    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
                        return false;
                    }
                }
                return true;
            }

        case taskSCHEDULER_NOT_STARTED:
        default:
            return false;
    }
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

bool delayUntilTick(TickType_t tick) {
    assert(!TT_IS_ISR());

    TickType_t tcnt, delay;

    tcnt = xTaskGetTickCount();

    /* Determine remaining number of tick to delay */
    delay = (TickType_t)tick - tcnt;

    /* Check if target tick has not expired */
    if ((delay != 0U) && (0 == (delay >> (8 * sizeof(TickType_t) - 1)))) {
        if (xTaskDelayUntil(&tcnt, delay) == pdPASS) {
            return true;
        }
    }

    return false;
}

TickType_t getTicks() {
    if (TT_IS_ISR() != 0U) {
        return xTaskGetTickCountFromISR();
    } else {
        return xTaskGetTickCount();
    }
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
