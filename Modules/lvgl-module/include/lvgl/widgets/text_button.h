// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a button without a container, for low-emphasis actions such as "Skip" or "Back".
 * It behaves like a regular button, but the theme styles it as a text button.
 * @param[in] parent the parent object for the new button
 * @return the created button
 */
lv_obj_t* lvgl_text_button_create(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
