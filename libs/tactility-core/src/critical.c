#include "critical.h"
#include "core_defines.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static portMUX_TYPE critical_mutex;

__TtCriticalInfo __tt_critical_enter(void) {
    __TtCriticalInfo info;

    info.isrm = 0;
    info.from_isr = TT_IS_ISR();
    info.kernel_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);

    if (info.from_isr) {
        info.isrm = taskENTER_CRITICAL_FROM_ISR();
    } else if (info.kernel_running) {
        taskENTER_CRITICAL(&critical_mutex);
    } else {
        portDISABLE_INTERRUPTS();
    }

    return info;
}

void __tt_critical_exit(__TtCriticalInfo info) {
    if (info.from_isr) {
        taskEXIT_CRITICAL_FROM_ISR(info.isrm);
    } else if (info.kernel_running) {
        taskEXIT_CRITICAL(&critical_mutex);
    } else {
        portENABLE_INTERRUPTS();
    }
}