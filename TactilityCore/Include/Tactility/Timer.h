#pragma once

#include "RtosCompatTimers.h"
#include "Thread.h"

#include <memory>
#include <functional>

namespace tt {

class Timer {

public:

    typedef std::function<void()> Callback;
    typedef void (*PendingCallback)(void* context, uint32_t arg);

private:

    struct TimerHandleDeleter {
        void operator()(TimerHandle_t handleToDelete) const {
            xTimerDelete(handleToDelete, portMAX_DELAY);
        }
    };

    Callback callback;
    std::unique_ptr<std::remove_pointer_t<TimerHandle_t>, TimerHandleDeleter> handle;

    static void onCallback(TimerHandle_t hTimer);

public:

    enum class Type {
        Once = 0,    ///< One-shot timer.
        Periodic = 1 ///< Repeating timer.
    };

    enum class Priority{
        Normal,   /**< Lower then other threads */
        Elevated, /**< Same as other threads */
    };

    /**
     * @param[in] type The timer type
     * @param[in] callback The callback function
     */
    Timer(Type type, Callback callback);

    ~Timer();

    /** Start timer
     * @warning This is asynchronous call, real operation will happen as soon as timer service process this request.
     * @param[in] interval The interval in ticks
     * @return success result
     */
    bool start(TickType_t interval);

    /** Restart timer with previous timeout value
     * @warning This is asynchronous call, real operation will happen as soon as timer service process this request.
     * @param[in] interval The interval in ticks
     * @return success result
     */
    bool restart(TickType_t interval);

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
    TickType_t getExpireTime();

    /**
     * Calls xTimerPendFunctionCall internally.
     * @param[in] callback the function to call
     * @param[in] callbackContext the first function argument
     * @param[in] callbackArg the second function argument
     * @param[in] timeout the function timeout (must set to 0 in ISR mode)
     * @return true on success
     */
    bool setPendingCallback(PendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeout);

    /** Set Timer thread priority
     * @param[in] priority The priority
     */
    void setThreadPriority(Thread::Priority priority);
};

} // namespace
