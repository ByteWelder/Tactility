#pragma once

#include "RtosCompatTask.h"

#include <functional>
#include <memory>
#include <string>

namespace tt {

typedef TaskHandle_t ThreadId;

class Thread final {

public:

    enum class State{
        Stopped,
        Starting,
        Running,
    };

    /** ThreadPriority */
    enum class Priority : UBaseType_t {
        None = 0U, /**< Uninitialized, choose system default */
        Idle = 1U,
        Lower = 2U,
        Low = 3U,
        Normal = 4U,
        High = 5U,
        Higher = 6U,
        Critical = 7U
    };


    /** ThreadCallback Your callback to run in new thread
     * @warning never use osThreadExit in Thread
     */
    typedef int32_t (*Callback)(void* context);
    typedef std::function<int32_t()> MainFunction;

    /** Write to stdout callback
     * @param[in] data pointer to data
     * @param[in] size data size @warning your handler must consume everything
     */
    typedef void (*StdoutWriteCallback)(const char* data, size_t size);

    /** Thread state change callback called upon thread state change
     * @param[in] state new thread state
     * @param[in] context callback context
     */
    typedef void (*StateCallback)(State state, void* context);

private:

    static void mainBody(void* context);

    TaskHandle_t taskHandle = nullptr;
    State state = State::Stopped;
    MainFunction mainFunction;
    int32_t callbackResult = 0;
    StateCallback stateCallback = nullptr;
    void* stateCallbackContext = nullptr;
    std::string name = {};
    Priority priority = Priority::Normal;
    configSTACK_DEPTH_TYPE stackSize = 0;
    portBASE_TYPE affinity = -1;

    void setState(Thread::State state);

public:

    Thread() = default;

    /** Allocate Thread, shortcut version
     * @param[in] name the name of the thread
     * @param[in] stackSize in bytes
     * @param[in] function
     * @param[in] affinity Which CPU core to pin this task to, -1 means unpinned (only works on ESP32)
     */
    Thread(
        std::string name,
        configSTACK_DEPTH_TYPE stackSize,
        MainFunction function,
        portBASE_TYPE affinity = -1
    );

    ~Thread();

    /** Set Thread name
     * @param[in] name string
     */
    void setName(std::string name);

    /** Set Thread stack size
     * @param[in] stackSize stack size in bytes
     */
    void setStackSize(size_t stackSize);

    /** Set CPU core pinning for this thread.
     * @param[in] affinity -1 means not pinned, otherwise it's the core id (e.g. 0 or 1 on ESP32)
     */
    void setAffinity(portBASE_TYPE affinity);

    /** Set Thread callback
     * @param[in] callback ThreadCallback, called upon thread run
     * @param[in] callbackContext what to pass to the callback
     */
    [[deprecated("use setMainFunction()")]]
    void setCallback(Callback callback, _Nullable void* callbackContext = nullptr);

    /** Set Thread callback
     * @param[in] function called upon thread run
     */
    void setMainFunction(MainFunction function);

    /** Set Thread priority
     * @param[in] priority ThreadPriority value
     */
    void setPriority(Priority priority);


    /** Set Thread state change callback
     * @param[in] callback state change callback
     * @param[in] callbackContext pointer to context
     */
    void setStateCallback(StateCallback callback, _Nullable void* callbackContext = nullptr);

    /** Get Thread state
     * @return thread state from ThreadState
     */
    State getState() const;

    /** Start Thread */
    void start();

    /** Join Thread
     * @warning make sure you manually interrupt any logic in your thread (e.g. by an EventFlag or boolean+Mutex)
     * @param[in] timeout the maximum amount of time to wait
     * @param[in] pollInterval the amount of ticks to wait before we check again if the thread is finished
     * @return success result
     */
    bool join(TickType_t timeout = portMAX_DELAY, TickType_t pollInterval = 10);

    /** Get FreeRTOS ThreadId for Thread instance
     * @return ThreadId or nullptr
     */
    ThreadId getId() const;

    /**
     * @warning crashes when state is not "stopped"
     * @return thread return code
     */
    int32_t getReturnCode() const;

    /** Suspend thread
     * @param[in] threadId thread id
     */
    static void suspend(ThreadId threadId);

    /** Resume thread
     * @param[in] threadId thread id
     */
    static void resume(ThreadId threadId);

    /** Get thread suspended state
     * @param[in] threadId thread id
     * @return true if thread is suspended
     */
    static bool isSuspended(ThreadId threadId);

    /**
     * @brief Get thread stack watermark
     * @param[in] threadId
     * @return uint32_t
     */
    static uint32_t getStackSpace(ThreadId threadId);

    /** @return pointer to Thread instance or nullptr if this thread doesn't belong to Tactility */
    static Thread* getCurrent();

    static uint32_t setFlags(ThreadId threadId, uint32_t flags);

    static uint32_t clearFlags(uint32_t flags);

    static uint32_t getFlags();

    static uint32_t awaitFlags(uint32_t flags, uint32_t options, uint32_t timeout);
};

constexpr auto THREAD_PRIORITY_SERVICE = Thread::Priority::High;
constexpr auto THREAD_PRIORITY_RENDER = Thread::Priority::Higher;
constexpr auto THREAD_PRIORITY_ISR = Thread::Priority::Critical;

} // namespace
