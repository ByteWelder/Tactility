#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
void tt_screenshot_start_timed(const char* path, uint8_t delay_in_seconds, uint8_t amount);

/** @brief Starts taking screenshot when an app is started
 * @param path the path to store the screenshots in
 */
void tt_screenshot_start_apps(const char* path);

void tt_screenshot_stop();

ScreenshotMode tt_screenshot_get_mode();

bool tt_screenshot_is_started();

#ifdef __cplusplus
}
#endif