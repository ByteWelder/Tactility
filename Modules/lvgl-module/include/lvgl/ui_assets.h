// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Amount of spinner images. The sizes in pixels are 12, 16, 20, 24, 32, 36 and 40 */
#define LVGL_UI_SPINNER_COUNT 7

/** Amount of cursor images. The sizes in pixels are 12, 16, 20, 24, 32 and 40 */
#define LVGL_UI_CURSOR_COUNT 6

/**
 * @brief PNG images for the spinner and the mouse cursor, ordered by size.
 * The arrays have LVGL_UI_SPINNER_COUNT and LVGL_UI_CURSOR_COUNT entries and must stay valid forever.
 */
struct LvglUiAssets {
    const lv_image_dsc_t* spinner;
    const lv_image_dsc_t* cursor;
};

/**
 * @brief Sets the images that the spinner and cursor getters pick from.
 * @param[in] assets the images, copied by reference
 */
void lvgl_ui_assets_configure(const struct LvglUiAssets* assets);

/**
 * @brief Gets the spinner image that fits the default text size best.
 * @return an image for lv_image_set_src(), or NULL when not configured
 */
const lv_image_dsc_t* lvgl_get_spinner_image(void);

/**
 * @brief Gets the mouse cursor image that fits the default text size best.
 * @return an image for lv_image_set_src(), or NULL when not configured
 */
const lv_image_dsc_t* lvgl_get_cursor_image(void);

#ifdef __cplusplus
}
#endif
