#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#pragma once

#include <cstdint>
#include <Thread.h>
#include <Mutex.h>

namespace tt::service::screenshot {

#define TASK_WORK_TYPE_DELAY 1
#define TASK_WORK_TYPE_APPS 2

#define SCREENSHOT_PATH_LIMIT 128

class ScreenshotTask {

    struct ScreenshotTaskWork {
        int type = TASK_WORK_TYPE_DELAY ;
        uint8_t delay_in_seconds = 0;
        uint8_t amount = 0;
        char path[SCREENSHOT_PATH_LIMIT] = { 0 };
    };

    Thread* thread = nullptr;
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    bool interrupted = false;
    bool finished = false;
    ScreenshotTaskWork work;

public:
    ScreenshotTask() = default;
    ~ScreenshotTask();

    /** @brief Start taking screenshots after a certain delay
     * @param task the screenshot task
     * @param path the path to store the screenshots at
     * @param delay_in_seconds the delay before starting (and between successive screenshots)
     * @param amount 0 = indefinite, >0 for a specific
     */
    void startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount);

    /** @brief Start taking screenshot whenever an app is started
     * @param task the screenshot task
     * @param path the path to store the screenshots at
     */
    void startApps(const char* path);

    /** @brief Stop taking screenshots
     * @param task the screenshot task
     */
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
