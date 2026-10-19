/*
 * FreeRTOS port for Emscripten (WebAssembly with pthreads).
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from FreeRTOS-Kernel's portable/ThirdParty/GCC/Posix port.c (MIT license,
 * Cambridge Consultants Ltd.), restructured for WebAssembly:
 *
 * Emscripten threads cannot be sent signals, so the original SIGALRM tick and
 * sigwait-based suspension are gone. Each task is a pthread parked on its own
 * latched condition event. The xSchedulerMutex mutex models "the CPU": it is
 * owned by whichever thread is currently executing kernel code, and it is never
 * held across a context switch. Suspending is always unlock-then-wait-then-relock;
 * a freshly started task waits first and locks after.
 *
 * Critical sections nest per task (a TLS counter): only the outermost
 * vPortEnterCritical/vPortExitCritical pair touches the mutex, so the counting
 * matches the kernel's own taskENTER_CRITICAL nesting.
 *
 * The tick comes from a dedicated thread that takes the same mutex once per tick
 * period. A task inside a critical section holds the mutex, which both defers the
 * tick until the critical section ends and guarantees the tick handler never runs
 * concurrently with kernel state mutation. The tick thread skips ticking while the
 * scheduler is suspended (vTaskSuspendAll); xTaskIncrementTick() pends a count for
 * any tick it cannot process, so the boundary race is benign.
 *
 * A task preempted by the tick cannot be stopped mid-user-code (wasm has no way to
 * interrupt a worker), so it keeps running until its next kernel entry:
 * vTaskSwitchContext() has already moved pxCurrentTCB, and the task's next
 * outermost vPortEnterCritical() sees that it is no longer the selected task and
 * parks it until it is selected again (deferred suspend).
 */

#include "portmacro.h"
#include "event.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/*-----------------------------------------------------------*/

typedef struct THREAD
{
    pthread_t pthread;
    TaskFunction_t pxCode;
    void * pvParams;
    BaseType_t xDying;
    struct event * ev;
} Thread_t;

/*
 * The additional per-thread data is stored at the beginning of the
 * task's stack.
 */
static inline Thread_t * prvGetThreadFromTask( TaskHandle_t xTask )
{
    StackType_t * pxTopOfStack = *( StackType_t ** ) xTask;

    return ( Thread_t * ) ( pxTopOfStack + 1 );
}

/*-----------------------------------------------------------*/

static pthread_mutex_t xSchedulerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t hThreadKeyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t xFreeRtosMarkerKey;
static pthread_key_t xSelfThreadKey;
static pthread_key_t xCriticalNestingKey;
static struct event * xSchedulerEndEvent;
static pthread_t hTimerTickThread;
static volatile BaseType_t xTimerTickThreadShouldRun;
static uint64_t prvStartTimeNs;
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void );
static void * prvWaitForStart( void * pvParams );
static void prvSwitchThread( Thread_t * xThreadToResume,
                             Thread_t * xThreadToSuspend );
static void prvSuspendSelf( Thread_t * thread );
static void prvResumeThread( Thread_t * xThreadId );
static void vPortStartFirstTask( void );
static void prvPortYieldFromISR( void );
static BaseType_t prvIsFreeRTOSThread( void );
/*-----------------------------------------------------------*/

static void prvThreadKeyDestructor( void * pvData )
{
    free( pvData );
}
/*-----------------------------------------------------------*/

static void prvInitThreadKeys( void )
{
    pthread_key_create( &xFreeRtosMarkerKey, prvThreadKeyDestructor );
    /* Self Thread_t pointers are owned by the task's stack, not by the key. */
    pthread_key_create( &xSelfThreadKey, NULL );
    pthread_key_create( &xCriticalNestingKey, prvThreadKeyDestructor );
    xSchedulerEndEvent = event_create();
}
/*-----------------------------------------------------------*/

static void prvMarkAsFreeRTOSThread( void )
{
    uint8_t * pucThreadData = NULL;

    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );

    pucThreadData = malloc( 1 );
    configASSERT( pucThreadData != NULL );

    *pucThreadData = 1;

    pthread_setspecific( xFreeRtosMarkerKey, pucThreadData );
}
/*-----------------------------------------------------------*/

static BaseType_t prvIsFreeRTOSThread( void )
{
    uint8_t * pucThreadData = NULL;

    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );

    pucThreadData = ( uint8_t * ) pthread_getspecific( xFreeRtosMarkerKey );

    return ( pucThreadData != NULL ) && ( *pucThreadData == 1 ) ? pdTRUE : pdFALSE;
}
/*-----------------------------------------------------------*/

static Thread_t * prvGetSelfThread( void )
{
    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );

    return ( Thread_t * ) pthread_getspecific( xSelfThreadKey );
}
/*-----------------------------------------------------------*/

static UBaseType_t prvGetCriticalNesting( void )
{
    UBaseType_t * nesting;

    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );

    nesting = ( UBaseType_t * ) pthread_getspecific( xCriticalNestingKey );

    return nesting != NULL ? *nesting : 0;
}
/*-----------------------------------------------------------*/

static void prvSetCriticalNesting( UBaseType_t value )
{
    UBaseType_t * nesting;

    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );

    nesting = ( UBaseType_t * ) pthread_getspecific( xCriticalNestingKey );
    if( nesting == NULL )
    {
        nesting = malloc( sizeof( UBaseType_t ) );
        configASSERT( nesting != NULL );
        pthread_setspecific( xCriticalNestingKey, nesting );
    }
    *nesting = value;
}
/*-----------------------------------------------------------*/

static void prvFatalError( const char * pcCall,
                           int iErrno ) __attribute__( ( __noreturn__ ) );

void prvFatalError( const char * pcCall,
                    int iErrno )
{
    fprintf( stderr, "%s: %d\n", pcCall, iErrno );
    abort();
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     StackType_t * pxEndOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    Thread_t * thread;
    pthread_attr_t xThreadAttributes;
    size_t ulStackSize;
    int iRet;

    /*
     * Store the additional thread data at the start of the stack.
     */
    thread = ( Thread_t * ) ( pxTopOfStack + 1 ) - 1;
    pxTopOfStack = ( StackType_t * ) thread - 1;

    /* Ensure that there is enough space to store Thread_t on the stack. */
    ulStackSize = ( size_t ) ( pxTopOfStack + 1 - pxEndOfStack ) * sizeof( *pxTopOfStack );
    configASSERT( ulStackSize > sizeof( Thread_t ) );
    ( void ) ulStackSize; /* suppress set but not used warning */

    thread->pxCode = pxCode;
    thread->pvParams = pvParameters;
    thread->xDying = pdFALSE;

    pthread_attr_init( &xThreadAttributes );

    thread->ev = event_create();

    /*
     * No scheduler lock here: the new thread parks on its own event before touching
     * anything, and that event is only signaled after the task is in a ready list.
     * Locking here would deadlock when a task is created from inside a critical
     * section (xSchedulerMutex is not recursive).
     */
    iRet = pthread_create( &thread->pthread, &xThreadAttributes,
                           prvWaitForStart, thread );

    if( iRet != 0 )
    {
        prvFatalError( "pthread_create", iRet );
    }

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void vPortStartFirstTask( void )
{
    Thread_t * pxFirstThread = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    /* Start the first task. */
    prvResumeThread( pxFirstThread );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
    ( void ) pthread_once( &hThreadKeyOnce, prvInitThreadKeys );
    prvMarkAsFreeRTOSThread();

    /*
     * Hold the scheduler mutex before the tick thread starts, so no tick can be
     * processed before the first task runs (this matches the interrupts-disabled
     * state xPortStartScheduler() is called in).
     */
    pthread_mutex_lock( &xSchedulerMutex );

    prvSetupTimerInterrupt();

    /* Start the first task. */
    vPortStartFirstTask();

    /*
     * The scheduler thread itself never runs task code. Hand the mutex to the
     * first task and park until vPortEndScheduler() (which normally never comes).
     */
    pthread_mutex_unlock( &xSchedulerMutex );
    event_wait( xSchedulerEndEvent );

    xTimerTickThreadShouldRun = pdFALSE;
    pthread_join( hTimerTickThread, NULL );

    return pdFALSE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    event_signal( xSchedulerEndEvent );

    /* This thread is deleted by the kernel right after; park until then. */
    prvSuspendSelf( prvGetSelfThread() );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    Thread_t * self;
    UBaseType_t nesting;

    nesting = prvGetCriticalNesting();
    if( nesting > 0 )
    {
        prvSetCriticalNesting( nesting + 1 );
        return;
    }

    pthread_mutex_lock( &xSchedulerMutex );

    /*
     * Deferred suspend: the tick thread may have selected another task while this
     * one was running user code (there is no preemption on wasm). Park here until
     * this task is selected again, so kernel state is only ever touched by the
     * currently selected task.
     */
    if( xTaskGetSchedulerState() != taskSCHEDULER_RUNNING )
    {
        prvSetCriticalNesting( 1 );
        return;
    }

    self = prvGetSelfThread();

    if( self == NULL )
    {
        prvSetCriticalNesting( 1 );
        return;
    }

    while( prvGetThreadFromTask( xTaskGetCurrentTaskHandle() )->pthread != self->pthread )
    {
        pthread_mutex_unlock( &xSchedulerMutex );
        event_wait( self->ev );
        pthread_mutex_lock( &xSchedulerMutex );
    }

    prvSetCriticalNesting( 1 );
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    UBaseType_t nesting;

    nesting = prvGetCriticalNesting();
    configASSERT( nesting > 0 );

    if( nesting > 1 )
    {
        prvSetCriticalNesting( nesting - 1 );
        return;
    }

    prvSetCriticalNesting( 0 );
    pthread_mutex_unlock( &xSchedulerMutex );
}
/*-----------------------------------------------------------*/

static void prvPortYieldFromISR( void )
{
    Thread_t * xThreadToSuspend;
    Thread_t * xThreadToResume;

    xThreadToSuspend = prvGetSelfThread();
    configASSERT( xThreadToSuspend != NULL );

    vTaskSwitchContext();

    xThreadToResume = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    prvSwitchThread( xThreadToResume, xThreadToSuspend );
}
/*-----------------------------------------------------------*/

void vPortYield( void )
{
    /* This must never be called from outside of a FreeRTOS-owned thread, or
     * the thread could get stuck in a suspended state. */
    configASSERT( prvIsFreeRTOSThread() == pdTRUE );

    vPortEnterCritical();

    prvPortYieldFromISR();

    vPortExitCritical();
}
/*-----------------------------------------------------------*/

void vPortDisableInterrupts( void )
{
    if( prvIsFreeRTOSThread() == pdTRUE )
    {
        vPortEnterCritical();
    }
}
/*-----------------------------------------------------------*/

void vPortEnableInterrupts( void )
{
    if( prvIsFreeRTOSThread() == pdTRUE )
    {
        vPortExitCritical();
    }
}
/*-----------------------------------------------------------*/

UBaseType_t xPortSetInterruptMask( void )
{
    vPortDisableInterrupts();
    return ( UBaseType_t ) 0;
}
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( UBaseType_t uxMask )
{
    ( void ) uxMask;
    vPortEnableInterrupts();
}
/*-----------------------------------------------------------*/

static uint64_t prvGetTimeNs( void )
{
    struct timespec t;

    clock_gettime( CLOCK_MONOTONIC, &t );

    return ( uint64_t ) t.tv_sec * ( uint64_t ) 1000000000UL + ( uint64_t ) t.tv_nsec;
}
/*-----------------------------------------------------------*/

static void * prvTimerTickHandler( void * arg )
{
    ( void ) arg;

    prvMarkAsFreeRTOSThread();

    while( xTimerTickThreadShouldRun == pdTRUE )
    {
        Thread_t * pxThreadToSuspend;
        Thread_t * pxThreadToResume;

        pthread_mutex_lock( &xSchedulerMutex );

        /* Skip the tick entirely while the scheduler is suspended (vTaskSuspendAll):
         * xTaskIncrementTick() would only pend a count we would then also advance. */
        if( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING )
        {
            pxThreadToSuspend = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

            if( xTaskIncrementTick() != pdFALSE )
            {
                /* Select next task. */
                vTaskSwitchContext();

                pxThreadToResume = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

                if( pxThreadToResume != pxThreadToSuspend )
                {
                    /*
                     * Do not suspend the preempted thread: it is running user code and
                     * cannot be stopped from outside. Resume the new task and leave the
                     * old one to park itself at its next kernel entry (deferred suspend,
                     * see vPortEnterCritical).
                     */
                    prvResumeThread( pxThreadToResume );
                }
            }
        }

        pthread_mutex_unlock( &xSchedulerMutex );
        usleep( portTICK_RATE_MICROSECONDS );
    }

    return NULL;
}
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void prvSetupTimerInterrupt( void )
{
    xTimerTickThreadShouldRun = pdTRUE;
    pthread_create( &hTimerTickThread, NULL, prvTimerTickHandler, NULL );

    prvStartTimeNs = prvGetTimeNs();
}
/*-----------------------------------------------------------*/

void vPortThreadDying( void * pxTaskToDelete,
                       volatile BaseType_t * pxPendYield )
{
    Thread_t * pxThread = prvGetThreadFromTask( pxTaskToDelete );

    ( void ) pxPendYield;

    pxThread->xDying = pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortCancelThread( void * pxTaskToDelete )
{
    Thread_t * pxThreadToCancel = prvGetThreadFromTask( pxTaskToDelete );

    /*
     * The thread has already been suspended so it can be safely cancelled.
     */
    pthread_cancel( pxThreadToCancel->pthread );
    event_signal( pxThreadToCancel->ev );
    pthread_join( pxThreadToCancel->pthread, NULL );
    event_delete( pxThreadToCancel->ev );
}
/*-----------------------------------------------------------*/

static void * prvWaitForStart( void * pvParams )
{
    Thread_t * pxThread = pvParams;

    prvMarkAsFreeRTOSThread();
    pthread_setspecific( xSelfThreadKey, pxThread );

    /*
     * Wait before locking: this thread does not own the scheduler mutex yet.
     * The signaler transfers ownership by unlocking after signaling.
     */
    event_wait( pxThread->ev );
    pthread_mutex_lock( &xSchedulerMutex );

    pthread_mutex_unlock( &xSchedulerMutex );

    /* Call the task's entry point. */
    pxThread->pxCode( pxThread->pvParams );

    /* A function that implements a task must not exit or attempt to return to
     * its caller as there is nothing to return to. If a task wants to exit it
     * should instead call vTaskDelete( NULL ). Artificially force an assert()
     * to be triggered if configASSERT() is defined, so application writers can
     * catch the error. */
    configASSERT( pdFALSE );

    return NULL;
}
/*-----------------------------------------------------------*/

static void prvSwitchThread( Thread_t * pxThreadToResume,
                             Thread_t * pxThreadToSuspend )
{
    if( pxThreadToSuspend != pxThreadToResume )
    {
        /*
         * Switch tasks. The scheduler mutex is still held by the suspending
         * thread: resume first, then release the mutex while suspending, so the
         * mutex is never held across the switch.
         */
        prvResumeThread( pxThreadToResume );

        if( pxThreadToSuspend->xDying == pdTRUE )
        {
            pthread_mutex_unlock( &xSchedulerMutex );
            pthread_exit( NULL );
        }
        else
        {
            prvSuspendSelf( pxThreadToSuspend );
        }
    }
}
/*-----------------------------------------------------------*/

static void prvSuspendSelf( Thread_t * thread )
{
    /*
     * Suspend this thread by waiting on its resume event. The scheduler mutex is
     * released while waiting and re-acquired before returning, so callers keep
     * their critical section balanced.
     */
    pthread_mutex_unlock( &xSchedulerMutex );
    event_wait( thread->ev );
    pthread_mutex_lock( &xSchedulerMutex );

    pthread_testcancel();
}
/*-----------------------------------------------------------*/

static void prvResumeThread( Thread_t * xThreadId )
{
    event_signal( xThreadId->ev );
}
/*-----------------------------------------------------------*/

uint32_t ulPortGetRunTime( void )
{
    struct tms xTimes;

    times( &xTimes );

    return ( uint32_t ) xTimes.tms_utime;
}
/*-----------------------------------------------------------*/
