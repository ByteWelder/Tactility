#include "Tactility/lvgl/Color.h"

lv_color_t lv_color_foreground() {
    return lv_color_make(0xFF, 0xFF, 0xFF);
}

lv_color_t lv_color_background() {
    return lv_color_make(0x28, 0x2B, 0x30);
}

lv_color_t lv_color_background_darkest() {
    return lv_color_make(0x00, 0x00, 0x00);
}
