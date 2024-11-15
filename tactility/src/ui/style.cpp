#include "style.h"

void tt_lv_obj_set_style_bg_blacken(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_color(obj, lv_color_black(), 0);
}

void tt_lv_obj_set_style_bg_invisible(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
}

void tt_lv_obj_set_style_no_padding(lv_obj_t* obj) {
    lv_obj_set_style_pad_all(obj, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_pad_gap(obj, LV_STATE_DEFAULT, 0);
}

void tt_lv_obj_set_style_auto_padding(lv_obj_t* obj) {
    lv_obj_set_style_pad_top(obj, 8, 0);
    lv_obj_set_style_pad_bottom(obj, 8, 0);
    lv_obj_set_style_pad_left(obj, 16, 0);
    lv_obj_set_style_pad_right(obj, 16, 0);
}
