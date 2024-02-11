#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void ScreenshotTask;

ScreenshotTask* screenshot_task_alloc();

void screenshot_task_free(ScreenshotTask* task);

/** @brief Start taking screenshots after a certain delay
 * @param task the screenshot task
 * @param path the path to store the screenshots at
 * @param delay_in_seconds the delay before starting (and between successive screenshots)
 * @param amount 0 = indefinite, >0 for a specific
 */
void screenshot_task_start_timed(ScreenshotTask* task, const char* path, uint8_t delay_in_seconds, uint8_t amount);

/** @brief Start taking screenshot whenever an app is started
 * @param task the screenshot task
 * @param path the path to store the screenshots at
 */
void screenshot_task_start_apps(ScreenshotTask* task, const char* path);

/** @brief Stop taking screenshots
 * @param task the screenshot task
 */
void screenshot_task_stop(ScreenshotTask* task);

#ifdef __cplusplus
}
#endif