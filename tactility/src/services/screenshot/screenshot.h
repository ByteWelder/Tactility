#pragma once

#include <cstdint>

namespace tt::service::screenshot {

typedef enum {
    ScreenshotModeNone,
    ScreenshotModeTimed,
    ScreenshotModeApps
} ScreenshotMode;

/** @brief Starts taking screenshot with a timer
 * @param path the path to store the screenshots in
 * @param delay_in_seconds the delay before starting (and between successive screenshots)
 * @param amount 0 = indefinite, >0 for a specific
 */
void start_timed(const char* path, uint8_t delay_in_seconds, uint8_t amount);

/** @brief Starts taking screenshot when an app is started
 * @param path the path to store the screenshots in
 */
void start_apps(const char* path);

void stop();

ScreenshotMode get_mode();

bool is_started();

} // namespace
