/*
 * FreeRTOS port for Emscripten (WebAssembly with pthreads).
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from FreeRTOS-Kernel's portable/ThirdParty/GCC/Posix portmacro.h (MIT),
 * with the signal-based pieces removed: WebAssembly threads cannot be interrupted,
 * so this port schedules cooperatively on top of a tick thread (see port.c).
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <limits.h>
#include <stdint.h>

/* Port specific definitions. */

#define portCHAR                 char
#define portFLOAT                float
#define portDOUBLE               double
#define portLONG                 long
#define portSHORT                short
#define portSTACK_TYPE           unsigned long
#define portBASE_TYPE            long
#define portPOINTER_SIZE_TYPE    intptr_t

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

typedef unsigned long    TickType_t;
#define portMAX_DELAY              ( ( TickType_t ) ULONG_MAX )

#define portTICK_TYPE_IS_ATOMIC    1


/* Architecture specifics. */
#define portSTACK_GROWTH                   ( -1 )
#define portHAS_STACK_OVERFLOW_CHECKING    ( 1 )
#define portTICK_PERIOD_MS                 ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portTICK_RATE_MICROSECONDS         ( ( TickType_t ) 1000000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT                 8

/* Scheduler utilities. */
extern void vPortYield( void );

#define portYIELD()                vPortYield()

#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do                                           \
    {                                            \
        if( xSwitchRequired != pdFALSE )         \
        {                                        \
            vPortYield();                        \
        }                                        \
    } while( 0 )
#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )

/* Critical section management. */
extern void vPortDisableInterrupts( void );
extern void vPortEnableInterrupts( void );
#define portSET_INTERRUPT_MASK()      ( vPortDisableInterrupts() )
#define portCLEAR_INTERRUPT_MASK()    ( vPortEnableInterrupts() )

extern UBaseType_t xPortSetInterruptMask( void );
extern void vPortClearInterruptMask( UBaseType_t xMask );

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()         xPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortClearInterruptMask( x )
#define portDISABLE_INTERRUPTS()                  portSET_INTERRUPT_MASK()
#define portENABLE_INTERRUPTS()                   portCLEAR_INTERRUPT_MASK()
#define portENTER_CRITICAL()                      vPortEnterCritical()
#define portEXIT_CRITICAL()                       vPortExitCritical()


extern void vPortThreadDying( void * pxTaskToDelete,
                              volatile BaseType_t * pxPendYield );
extern void vPortCancelThread( void * pxTaskToDelete );
#define portPRE_TASK_DELETE_HOOK( pvTaskToDelete, pxPendYield )    vPortThreadDying( ( pvTaskToDelete ), ( pxPendYield ) )
#define portCLEAN_UP_TCB( pxTCB )                                  vPortCancelThread( pxTCB )

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )         void vFunction( void * pvParameters ) __attribute__( ( noreturn ) )
#define portTASK_FUNCTION( vFunction, pvParameters )               void vFunction( void * pvParameters )

/*
 * Tasks run in their own pthreads and context switches between them are always
 * a full memory barrier, so only a compiler barrier is needed to stop the
 * compiler reordering.
 */
#define portMEMORY_BARRIER()                        __asm volatile ( "" ::: "memory" )

/* This is an emulated port: the tick thread can run concurrently with a task that is
 * suspending the scheduler, so uxSchedulerSuspended needs a real barrier around it. */
#define portSOFTWARE_BARRIER()                      __atomic_thread_fence( __ATOMIC_SEQ_CST )

extern uint32_t ulPortGetRunTime( void );
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()    /* no-op */
#define portGET_RUN_TIME_COUNTER_VALUE()            ulPortGetRunTime()

#ifdef __cplusplus
    }
#endif

#endif /* PORTMACRO_H */
