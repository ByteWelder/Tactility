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
     *
     * @warning This is asynchronous call, real operation will happen as soon as
     *          timer service process this request.
     *
     * @param[in] ticks The interval in ticks
     * @return The status.
     */
    TtStatus start(uint32_t ticks);

    /** Restart timer with previous timeout value
     *
     * @warning    This is asynchronous call, real operation will happen as soon as
     *             timer service process this request.
     *
     * @param[in] ticks The interval in ticks
     *
     * @return The status.
     */
    TtStatus restart(uint32_t ticks);


    /** Stop timer
     *
     * @warning    This is asynchronous call, real operation will happen as soon as
     *             timer service process this request.
     *
     * @return The status.
     */
    TtStatus stop();

    /** Is timer running
     *
     * @warning    This cal may and will return obsolete timer state if timer
     *             commands are still in the queue. Please read FreeRTOS timer
     *             documentation first.
     *
     * @return true when running
     */
    bool isRunning();

    /** Get timer expire time
     *
     * @param instance The Timer instance
     *
     * @return expire tick
     */
    uint32_t getExpireTime();

    void pendingCallback(PendingCallback callback, void* callbackContext, uint32_t arg);

    typedef enum {
        TimerThreadPriorityNormal,   /**< Lower then other threads */
        TimerThreadPriorityElevated, /**< Same as other threads */
    } TimerThreadPriority;

    /** Set Timer thread priority
     *
     * @param[in] priority The priority
     */
    void setThreadPriority(TimerThreadPriority priority);
};

} // namespace
