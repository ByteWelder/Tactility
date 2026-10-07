// SPDX-License-Identifier: Apache-2.0

// LVGL's default theme derives every pad/margin, radius and border from the display DPI:
//     LV_DPX_CALC(disp_dpi, n)
// This board reports its physical panel DPI (`display.dpi` = 326), so with stock settings the
// theme's metrics are scaled for a 326-DPI font while the actual text is rendered at its
// configured pixel size (`lvgl.fontSize` = 22). The result is padding that is far too large
// relative to the text.
//
// Tactility replaces the theme font with a dynamically generated one (Appearance settings), and
// the Appearance app restarts LVGL when that changes, so the display and theme are recreated on
// every font change. This board-local hook therefore runs on every lv_display_create() and pins the
// display DPI to the loaded text font, so the theme's padding and other DPI-derived metrics stay
// proportional to the text. It is a link-time wrapper - the same mechanism Tactility uses for its
// own lv_*_create() hooks - so neither core Tactility nor LVGL source is modified.

#include <tactility/log.h>

#include <lvgl/fonts.h>
#include <lvgl/lvgl.h>

constexpr auto* TAG = "waveshare-s3-touch-amoled-206";

namespace {

// The default theme's medium padding is PAD_DEF = LV_DPX_CALC(disp_dpi, 20) ~= disp_dpi / 8.
// Using disp_dpi = font_size makes that padding ~= font_size / 8, i.e. 3 px at the default 22 px
// font - a very tight layout, while still tracking the text size.
constexpr int32_t DISPLAY_DPI_PER_FONT_PIXEL = 1;

constexpr int32_t displayDpiForFontSize(int32_t font_size) {
    return font_size * DISPLAY_DPI_PER_FONT_PIXEL;
}

} // namespace

extern "C" {

lv_display_t* __real_lv_display_create(int32_t hor_res, int32_t ver_res);

// By the time lv_display_create() returns, the default theme has already been initialised with the
// stock DPI. Setting the DPI and re-sending LV_EVENT_RESOLUTION_CHANGED makes that theme rebuild
// its DPI-scaled styles (see lv_theme_default.c's resolution_change_event_cb) and report the change
// so the freshly created screen re-layouts. Runs with the LVGL lock held by lvgl_devices_attach().
lv_display_t* __wrap_lv_display_create(int32_t hor_res, int32_t ver_res) {
    lv_display_t* display = __real_lv_display_create(hor_res, ver_res);
    if (display == nullptr) {
        return display;
    }

    const int32_t font_size = static_cast<int32_t>(lvgl_get_text_font_height(FONT_SIZE_DEFAULT));
    if (font_size <= 0) {
        return display;
    }

    const int32_t dpi = displayDpiForFontSize(font_size);
    if (lv_display_get_dpi(display) != dpi) {
        lv_display_set_dpi(display, dpi);
        lv_display_send_event(display, LV_EVENT_RESOLUTION_CHANGED, nullptr);
        LOG_I(TAG, "LVGL display DPI set to %d for text font %d px", (int)dpi, (int)font_size);
    }

    return display;
}

} // extern "C"
