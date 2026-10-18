// SPDX-License-Identifier: Apache-2.0
#include <lvgl/ui_assets.h>
#include <lvgl/fonts.h>

#include <lvgl.h>

#include <stdlib.h>

// Must match the sizes in Buildscripts/generate-ui-assets.py
static const int32_t spinner_sizes[LVGL_UI_SPINNER_COUNT] = { 12, 16, 20, 24, 32, 36, 40 };
static const int32_t cursor_sizes[LVGL_UI_CURSOR_COUNT] = { 12, 16, 20, 24, 32, 40 };

static struct LvglUiAssets ui_assets;

void lvgl_ui_assets_configure(const struct LvglUiAssets* assets) {
    ui_assets = *assets;
}

/** @return the index of the size that is the closest to target_size */
static int nearest_size(const int32_t* sizes, int count, int32_t target_size) {
    int nearest = 0;
    for (int i = 1; i < count; i++) {
        if (abs(sizes[i] - target_size) < abs(sizes[nearest] - target_size)) {
            nearest = i;
        }
    }
    return nearest;
}

const lv_image_dsc_t* lvgl_get_spinner_image(void) {
    if (ui_assets.spinner == NULL) {
        return NULL;
    }
    const int32_t text_size = (int32_t)lvgl_get_text_font_height(FONT_SIZE_DEFAULT);
    return &ui_assets.spinner[nearest_size(spinner_sizes, LVGL_UI_SPINNER_COUNT, text_size * 3 / 2)];
}

const lv_image_dsc_t* lvgl_get_cursor_image(void) {
    if (ui_assets.cursor == NULL) {
        return NULL;
    }
    const int32_t text_size = (int32_t)lvgl_get_text_font_height(FONT_SIZE_DEFAULT);
    return &ui_assets.cursor[nearest_size(cursor_sizes, LVGL_UI_CURSOR_COUNT, text_size * 3 / 2)];
}
