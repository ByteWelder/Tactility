#include "top_bar.h"
#include "widgets.h"

void top_bar(lv_obj_t* parent) {
    lv_obj_t* topbar_container = lv_obj_create(parent);
    lv_obj_set_width(topbar_container, LV_PCT(100));
    lv_obj_set_height(topbar_container, TOP_BAR_HEIGHT);
    lv_obj_set_style_no_padding(topbar_container);
    lv_obj_set_style_bg_blacken(topbar_container);
    lv_obj_center(topbar_container);
    lv_obj_set_flex_flow(topbar_container, LV_FLEX_FLOW_ROW);

    lv_obj_t* spacer = lv_obj_create(topbar_container);
    lv_obj_set_height(spacer, LV_PCT(100));
    lv_obj_set_style_no_padding(spacer);
    lv_obj_set_style_bg_blacken(spacer);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t* wifi = lv_img_create(topbar_container);
    lv_obj_set_size(wifi, TOP_BAR_ICON_SIZE, TOP_BAR_ICON_SIZE);
    lv_obj_set_style_no_padding(wifi);
    lv_obj_set_style_bg_blacken(wifi);
    lv_obj_set_style_img_recolor(wifi, lv_color_white(), 0);
    lv_obj_set_style_img_recolor_opa(wifi, 255, 0);
    lv_img_set_src(wifi, "A:/assets/ic_small_wifi_off.png");
}