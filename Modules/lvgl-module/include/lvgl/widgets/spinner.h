// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the Tactility spinner widget.
 * @param[in] parent the parent object for the new spinner
 * @return the created spinner
 */
lv_obj_t* lvgl_spinner_create(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
