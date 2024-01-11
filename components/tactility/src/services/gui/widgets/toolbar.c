#include "toolbar.h"

#include "services/loader/loader.h"
#include "ui/spacer.h"
#include "ui/style.h"

static void app_toolbar_close(lv_event_t* event) {
    loader_stop_app();
}

lv_obj_t* tt_lv_toolbar_create(lv_obj_t* parent, lv_coord_t offset_y, const AppManifest* manifest) {
    lv_obj_t* toolbar = lv_obj_create(parent);
    lv_obj_set_width(toolbar, LV_PCT(100));
    lv_obj_set_height(toolbar, TOOLBAR_HEIGHT);
    lv_obj_set_pos(toolbar, 0, offset_y);
    tt_lv_obj_set_style_no_padding(toolbar);
    lv_obj_center(toolbar);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);

    lv_obj_t* close_button = lv_btn_create(toolbar);
    lv_obj_set_size(close_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    tt_lv_obj_set_style_no_padding(close_button);
    lv_obj_add_event_cb(close_button, &app_toolbar_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t* close_button_image = lv_img_create(close_button);
    lv_img_set_src(close_button_image, LV_SYMBOL_CLOSE);
    lv_obj_align(close_button_image, LV_ALIGN_CENTER, 0, 0);

    // Need spacer to avoid button press glitch animation
    tt_lv_spacer_create(toolbar, 2, 1);

    lv_obj_t* label_container = lv_obj_create(toolbar);
    tt_lv_obj_set_style_no_padding(label_container);
    lv_obj_set_style_border_width(label_container, 0, 0);
    lv_obj_set_height(label_container, LV_PCT(100)); // 2% less due to 4px translate (it's not great, but it works)
    lv_obj_set_flex_grow(label_container, 1);

    lv_obj_t* label = lv_label_create(label_container);
    lv_label_set_text(label, manifest->name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_size(label, LV_PCT(100), TOOLBAR_FONT_HEIGHT);
    lv_obj_set_pos(label, 0, (TOOLBAR_HEIGHT - TOOLBAR_FONT_HEIGHT - 10) / 2);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    return toolbar;
}
