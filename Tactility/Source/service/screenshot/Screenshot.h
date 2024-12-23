#pragma once

#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include <cstdint>

namespace tt::service::screenshot {

typedef enum {
    ScreenshotModeNone,
    ScreenshotModeTimed,
    ScreenshotModeApps
} Mode;

/** @brief Starts taking screenshot with a timer
 * @param path the path to store the screenshots in
 * @param delay_in_seconds the delay before starting (and between successive screenshots)
 * @param amount 0 = indefinite, >0 for a specific
 */
void startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount);

/** @brief Starts taking screenshot when an app is started
 * @param path the path to store the screenshots in
 */
void startApps(const char* path);

void stop();

Mode getMode();

bool isStarted();

} // namespace

#endif
