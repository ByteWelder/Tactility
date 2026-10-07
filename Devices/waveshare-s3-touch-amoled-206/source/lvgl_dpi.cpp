// SPDX-License-Identifier: Apache-2.0

// Tactility's Material themes (Modules/lvgl-module) derive their metrics - padding, gaps, radius,
// borders, row heights - from the display DPI:
//     LV_DPX_CALC(theme->disp_dpi, n)
// This board reports its physical panel DPI (`display.dpi` = 326), so with stock settings those
// metrics are scaled for a 326-DPI font while the text is rendered at its configured pixel size
// (`lvgl.fontSize` = 22). The result is spacing that is far too large relative to the text.
//
// Tactility replaces the theme font with a dynamically generated one (Appearance settings), and the
// Appearance app restarts LVGL when that changes, so the display and theme are recreated on every
// font change. This board-local hook pins the display DPI to the loaded text font on every
// lv_display_create(), so the theme's metrics stay proportional to the text. The theme itself is
// applied afterwards (lvgl_display_apply_theme() runs right after lv_display_create()), so it reads
// the DPI set here.
//
// It is a link-time wrapper - the same mechanism Tactility uses elsewhere - so neither core
// Tactility nor LVGL source is modified.

#include <tactility/log.h>

#include <lvgl/fonts.h>
#include <lvgl/lvgl.h>

constexpr auto* TAG = "waveshare-s3-touch-amoled-206";

namespace {

// The Material themes scale their spacing by disp_dpi / 160 (via LV_DPX_CALC). Using
// disp_dpi = font_size makes the base spacing about font_size / 8 - e.g. the largest control
// padding is ~3 px at the default 22 px font - while still tracking the text size.
constexpr int32_t DISPLAY_DPI_PER_FONT_PIXEL = 1;

constexpr int32_t displayDpiForFontSize(int32_t font_size) {
    return font_size * DISPLAY_DPI_PER_FONT_PIXEL;
}

} // namespace

extern "C" {

lv_display_t* __real_lv_display_create(int32_t hor_res, int32_t ver_res);

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
        LOG_I(TAG, "LVGL display DPI set to %d for text font %d px", (int)dpi, (int)font_size);
    }

    return display;
}

} // extern "C"
