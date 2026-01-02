#include <Tactility/TactilityConfig.h>

#if TT_FEATURE_SCREENSHOT_ENABLED

#pragma once

#include <Tactility/Thread.h>
#include <Tactility/RecursiveMutex.h>

namespace tt::service::screenshot {

#define TASK_WORK_TYPE_DELAY 1
#define TASK_WORK_TYPE_APPS 2

class ScreenshotTask {

    struct ScreenshotTaskWork {
        int type = TASK_WORK_TYPE_DELAY ;
        uint8_t delay_in_seconds = 0;
        uint8_t amount = 0;
        std::string path;
    };

    Thread* thread = nullptr;
    RecursiveMutex mutex;
    bool interrupted = false;
    bool finished = false;
    ScreenshotTaskWork work;

public:
    ScreenshotTask() = default;
    ~ScreenshotTask();

    /** @brief Start taking screenshots after a certain delay
     * @param[in] path the path to store the screenshots at
     * @param[in] delayInSeconds the delay before starting (and between successive screenshots)
     * @param[in] amount 0 = indefinite, >0 for a specific
     */
    void startTimed(const std::string& path, uint8_t delayInSeconds, uint8_t amount);

    /** @brief Start taking screenshot whenever an app is started
     * @param[in] path the path to store the screenshots at
     */
    void startApps(const std::string& path);

    /** @brief Stop taking screenshots */
    void stop();

    void taskMain();

    bool isFinished();

private:

    bool isInterrupted();
    void setFinished();
    void taskStart();
};

}

#endif
