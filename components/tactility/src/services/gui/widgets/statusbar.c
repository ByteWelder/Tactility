#include "statusbar.h"

#include "ui/spacer.h"
#include "ui/style.h"

lv_obj_t* tt_lv_statusbar_create(lv_obj_t* parent) {
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_height(wrapper, STATUSBAR_HEIGHT);
    tt_lv_obj_set_style_no_padding(wrapper);
    tt_lv_obj_set_style_bg_blacken(wrapper);
    lv_obj_center(wrapper);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);

    lv_obj_t* left_spacer = tt_lv_spacer_create(wrapper, 1, 1);
    lv_obj_set_flex_grow(left_spacer, 1);

    lv_obj_t* wifi = lv_img_create(wrapper);
    lv_obj_set_size(wifi, STATUSBAR_ICON_SIZE, STATUSBAR_ICON_SIZE);
    tt_lv_obj_set_style_no_padding(wifi);
    tt_lv_obj_set_style_bg_blacken(wifi);
    lv_obj_set_style_img_recolor(wifi, lv_color_white(), 0);
    lv_obj_set_style_img_recolor_opa(wifi, 255, 0);
    lv_img_set_src(wifi, "A:/assets/ic_small_wifi_off.png");

    return wrapper;
}