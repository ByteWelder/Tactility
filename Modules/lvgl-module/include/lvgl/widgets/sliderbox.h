// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a SliderBox: a slider flanked by "-" and "+" buttons with a value label.
 * @param[in] parent the parent object
 * @param[in] min minimum value (inclusive)
 * @param[in] max maximum value (inclusive)
 * @param[in] step the amount each +/- button press changes the value by
 * @param[in] value the initial value
 * @return the created SliderBox instance
 */
lv_obj_t* lvgl_sliderbox_create(lv_obj_t* parent, int32_t min, int32_t max, int32_t step, int32_t value);

/** @return the current value of the SliderBox */
int32_t lvgl_sliderbox_get_value(lv_obj_t* obj);

/**
 * Set the current value of the SliderBox (clamped to its [min, max] range).
 * Updates the slider position and the value label.
 */
void lvgl_sliderbox_set_value(lv_obj_t* obj, int32_t value, lv_anim_enable_t anim);

/**
 * Register a callback that is fired with LV_EVENT_VALUE_CHANGED whenever the value
 * changes, whether the change came from the slider or from the +/- buttons.
 * @param[in] obj the SliderBox instance
 * @param[in] callback the callback to invoke
 * @param[in] userData user data that is attached to the event
 */
void lvgl_sliderbox_add_value_changed_cb(lv_obj_t* obj, lv_event_cb_t callback, void* userData);

#ifdef __cplusplus
}
#endif
