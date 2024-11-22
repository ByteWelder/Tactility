#include "Critical.h"
#include "CoreDefines.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#endif

#ifdef ESP_PLATFORM
static portMUX_TYPE critical_mutex;
#define TT_ENTER_CRITICAL() taskENTER_CRITICAL(&critical_mutex)
#else
#define TT_ENTER_CRITICAL() taskENTER_CRITICAL()
#endif

namespace tt::critical {

TtCriticalInfo enter() {
    TtCriticalInfo info;

    info.isrm = 0;
    info.from_isr = TT_IS_ISR();
    info.kernel_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);

    if (info.from_isr) {
        info.isrm = taskENTER_CRITICAL_FROM_ISR();
    } else if (info.kernel_running) {
        TT_ENTER_CRITICAL();
    } else {
        portDISABLE_INTERRUPTS();
    }

    return info;
}

void exit(TtCriticalInfo info) {
    if (info.from_isr) {
        taskEXIT_CRITICAL_FROM_ISR(info.isrm);
    } else if (info.kernel_running) {
        TT_ENTER_CRITICAL();
    } else {
        portENABLE_INTERRUPTS();
    }
}

}
