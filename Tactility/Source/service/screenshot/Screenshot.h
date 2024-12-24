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
    Mutex mutex;
    std::unique_ptr<ScreenshotTask> task;
    Mode mode = ScreenshotModeNone;

public:

    bool isTaskStarted();
    Mode getMode();
    void startApps(const char* path);
    void startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount);
    void stop();
};

std::shared_ptr<ScreenshotService> _Nullable optScreenshotService();

} // namespace

#endif
