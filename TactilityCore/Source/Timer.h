#pragma once

#include "CoreTypes.h"

#include "RtosCompatTimers.h"
#include <memory>

namespace tt {

class Timer {
private:
    TimerHandle_t timerHandle;
public:

    typedef void (*Callback)(std::shared_ptr<void> context);
    typedef void (*PendingCallback)(void* context, uint32_t arg);

    Callback callback;
    std::shared_ptr<void> callbackContext;

    typedef enum {
        TypeOnce = 0,    ///< One-shot timer.
        TypePeriodic = 1 ///< Repeating timer.
    } Type;

    /**
     * @param[in] type The timer type
     * @param[in] callback The callback function
     * @param callbackContext The callback context
     */
    Timer(Type type, Callback callback, std::shared_ptr<void> callbackContext);

    ~Timer();

    /** Start timer
     * @warning This is asynchronous call, real operation will happen as soon as timer service process this request.
     * @param[in] ticks The interval in ticks
     * @return success result
     */
    bool start(uint32_t intervalTicks);

    /** Restart timer with previous timeout value
     * @warning This is asynchronous call, real operation will happen as soon as timer service process this request.
     * @param[in] ticks The interval in ticks
     * @return success result
     */
    bool restart(uint32_t intervalTicks);

    /** Stop timer
     * @warning This is asynchronous call, real operation will happen as soon as timer service process this request.
     * @return success result
     */
    bool stop();

    /** Is timer running
     * @warning This cal may and will return obsolete timer state if timer commands are still in the queue. Please read FreeRTOS timer documentation first.
     * @return true when running
     */
    bool isRunning();

    /** Get timer expire time
     * @return expire tick
     */
    uint32_t getExpireTime();

    /**
     * Calls xTimerPendFunctionCall internally.
     * @param[in] callback the function to call
     * @param[in] callbackContext the first function argument
     * @param[in] callbackArg the second function argument
     * @param[in] timeout the function timeout (must set to 0 in ISR mode)
     */
    bool setPendingCallback(PendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeout);

    typedef enum {
        TimerThreadPriorityNormal,   /**< Lower then other threads */
        TimerThreadPriorityElevated, /**< Same as other threads */
    } ThreadPriority;

    /** Set Timer thread priority
     * @param[in] priority The priority
     */
    void setThreadPriority(ThreadPriority priority);
};

} // namespace
