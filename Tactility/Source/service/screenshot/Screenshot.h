#pragma once

#include "Mutex.h"
#include "ScreenshotTask.h"
#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include <cstdint>

namespace tt::service::screenshot {

typedef enum {
    ScreenshotModeNone,
    ScreenshotModeTimed,
    ScreenshotModeApps
} Mode;

class ScreenshotService {

private:

    Mutex mutex;
    std::unique_ptr<ScreenshotTask> task;
    Mode mode = ScreenshotModeNone;

public:

    bool isTaskStarted();

    /** The state of the service. */
    Mode getMode();

    /** @brief Start taking screenshot whenever an app is started
     * @param[in] path the path to store the screenshots at
     */
    void startApps(const char* path);

    /** @brief Start taking screenshots after a certain delay
     * @param[in] path the path to store the screenshots at
     * @param[in] delayInSeconds the delay before starting (and between successive screenshots)
     * @param[in] amount 0 = indefinite, >0 for a specific
     */
    void startTimed(const char* path, uint8_t delayInSeconds, uint8_t amount);

    /** @brief Stop taking screenshots */
    void stop();
};

std::shared_ptr<ScreenshotService> _Nullable optScreenshotService();

} // namespace

#endif
