// SPDX-License-Identifier: Apache-2.0

// Tactility's Material themes (Modules/lvgl-module) derive both widget metrics (switch/checkbox/
// slider sizes, radius, borders, ...) and spacing (padding/gaps) from the display DPI. On this
// high-DPI panel (`display.dpi` = 326) those metrics are far too large relative to the text, but
// simply lowering the DPI to tighten the spacing also shrinks the widgets to unusable sizes - the
// switch track, for example, is LV_DPX_CALC(dpi, 52) x LV_DPX_CALC(dpi, 32).
//
// So the two are decoupled here, board-locally, with link-time wrappers (the same mechanism used
// elsewhere) instead of editing core Tactility/LVGL source:
//   - lv_display_create(): the display DPI tracks the text font, so widget sizes stay usable;
//   - lv_style_set_pad_*(): the theme's style paddings/gaps are divided, so spacing stays tight.
// Only the two theme files use style-based padding (apps use lv_obj_set_style_*, which does not go
// through these setters), so the padding wrappers affect the theme and nothing else.

#include <tactility/log.h>

#include <lvgl/fonts.h>
#include <lvgl/lvgl.h>

constexpr auto* TAG = "waveshare-s3-touch-amoled-206";

namespace {

// Widget metrics scale with the display DPI; this puts them near Material's baseline at the
// default 22 px font (e.g. the switch track is ~50 x 31 px).
constexpr int32_t WIDGET_DPI_PER_FONT_PIXEL = 7;

// Spacing is divided by this instead of following the widget DPI (e.g. the theme's largest control
// padding, dp(24), becomes ~3 px).
constexpr int32_t PADDING_DIVISOR = 8;

int32_t scalePadding(int32_t value) {
    // Keep zero and negative values (handle/knob geometry) as the theme designed them.
    if (value <= 0) {
        return value;
    }
    return (value + PADDING_DIVISOR / 2) / PADDING_DIVISOR;
}

int32_t displayDpiForFontSize(int32_t font_size) {
    return font_size * WIDGET_DPI_PER_FONT_PIXEL;
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

// lv_style_set_pad_all/hor/ver/gap are static inline and call these base setters, so wrapping the
// six base ones covers every padding a theme style sets.
#define WRAP_PADDING_SETTER(name) \
    void __real_##name(lv_style_t* style, int32_t value); \
    void __wrap_##name(lv_style_t* style, int32_t value) { __real_##name(style, scalePadding(value)); }

WRAP_PADDING_SETTER(lv_style_set_pad_top)
WRAP_PADDING_SETTER(lv_style_set_pad_bottom)
WRAP_PADDING_SETTER(lv_style_set_pad_left)
WRAP_PADDING_SETTER(lv_style_set_pad_right)
WRAP_PADDING_SETTER(lv_style_set_pad_row)
WRAP_PADDING_SETTER(lv_style_set_pad_column)

#undef WRAP_PADDING_SETTER

} // extern "C"
