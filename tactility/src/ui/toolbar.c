#include "toolbar.h"

#include "services/loader/loader.h"
#include "ui/spacer.h"
#include "ui/style.h"

#include "lvgl.h"

#define TOOLBAR_HEIGHT 40
#define TOOLBAR_FONT_HEIGHT 18

static void on_nav_pressed(lv_event_t* event) {
    NavAction action = (NavAction)event->user_data;
    action();
}

lv_obj_t* tt_lv_toolbar_create(lv_obj_t* parent, const Toolbar* toolbar) {
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_height(wrapper, TOOLBAR_HEIGHT);
    tt_lv_obj_set_style_no_padding(wrapper);
    lv_obj_center(wrapper);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);

    lv_obj_t* close_button = lv_btn_create(wrapper);
    lv_obj_set_size(close_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    tt_lv_obj_set_style_no_padding(close_button);
    lv_obj_add_event_cb(close_button, &on_nav_pressed, LV_EVENT_CLICKED, toolbar->nav_action);
    lv_obj_t* close_button_image = lv_img_create(close_button);
    lv_img_set_src(close_button_image, toolbar->nav_icon); // e.g. LV_SYMBOL_CLOSE
    lv_obj_align(close_button_image, LV_ALIGN_CENTER, 0, 0);

    // Need spacer to avoid button press glitch animation
    tt_lv_spacer_create(wrapper, 2, 1);

    lv_obj_t* label_container = lv_obj_create(wrapper);
    tt_lv_obj_set_style_no_padding(label_container);
    lv_obj_set_style_border_width(label_container, 0, 0);
    lv_obj_set_height(label_container, LV_PCT(100)); // 2% less due to 4px translate (it's not great, but it works)
    lv_obj_set_flex_grow(label_container, 1);

    lv_obj_t* title_label = lv_label_create(label_container);
    lv_label_set_text(title_label, toolbar->title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0); // TODO replace with size 18
    lv_obj_set_size(title_label, LV_PCT(100), TOOLBAR_FONT_HEIGHT);
    lv_obj_set_pos(title_label, 0, (TOOLBAR_HEIGHT - TOOLBAR_FONT_HEIGHT - 10) / 2);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);

    return wrapper;
}
