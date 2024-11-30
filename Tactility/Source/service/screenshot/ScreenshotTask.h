#pragma once

#include <cstdint>

namespace tt::service::screenshot::task {

typedef void ScreenshotTask;

ScreenshotTask* alloc();

void free(ScreenshotTask* task);

/** @brief Start taking screenshots after a certain delay
 * @param task the screenshot task
 * @param path the path to store the screenshots at
 * @param delay_in_seconds the delay before starting (and between successive screenshots)
 * @param amount 0 = indefinite, >0 for a specific
 */
void startTimed(ScreenshotTask* task, const char* path, uint8_t delay_in_seconds, uint8_t amount);

/** @brief Start taking screenshot whenever an app is started
 * @param task the screenshot task
 * @param path the path to store the screenshots at
 */
void startApps(ScreenshotTask* task, const char* path);

/** @brief Stop taking screenshots
 * @param task the screenshot task
 */
void stop(ScreenshotTask* task);

}
