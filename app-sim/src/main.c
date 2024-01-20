#include "FreeRTOS.h"
#include "log.h"
#include "portmacro.h"
#include "tactility.h"
#include "task.h"

void vAssertCalled(TT_UNUSED unsigned long line, TT_UNUSED const char* const file) {
    static portBASE_TYPE xPrinted = pdFALSE;
    volatile uint32_t set_to_nonzero_in_debugger_to_continue = 0;

    taskENTER_CRITICAL();
    {
        // Step out by attaching a debugger and setting set_to_nonzero_in_debugger_to_continue
        while (set_to_nonzero_in_debugger_to_continue == 0) { /* NO-OP */
        }
    }
    taskEXIT_CRITICAL();
}

int main() {
//    static const Config config = {
//        .hardware = NULL,
//        .apps = {
//            &hello_world_app
//        },
//        .services = { },
//        .auto_start_app_id = NULL
//    };
//
//    tactility_start(&config);
    TT_LOG_I("app", "Hello, world!");
    return 0;
}
