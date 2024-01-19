#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"


void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    static portBASE_TYPE xPrinted = pdFALSE;
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Parameters are not used. */
    ( void ) ulLine;
    ( void ) pcFileName;

    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
        value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
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
