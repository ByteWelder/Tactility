// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a step indicator: a horizontal row of line segments, one per step, that shows the progress through a number of steps.
 * The segments share the indicator's width. The theme determines their thickness, the gap between them, and the colours of finished and unfinished steps.
 * @param[in] parent the parent object
 * @return the created step indicator
 */
lv_obj_t* lvgl_step_indicator_create(lv_obj_t* parent);

/**
 * @brief Sets the number of steps, which is the number of segments.
 * @param[in] obj the step indicator
 * @param[in] count the number of steps
 */
void lvgl_step_indicator_set_step_count(lv_obj_t* obj, uint32_t count);

/**
 * @brief Sets how many steps are finished: the segments of the first @p finished_count steps show as finished.
 * @param[in] obj the step indicator
 * @param[in] finished_count the number of finished steps
 */
void lvgl_step_indicator_set_finished_count(lv_obj_t* obj, uint32_t finished_count);

#ifdef __cplusplus
}
#endif
