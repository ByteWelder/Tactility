#include "wifi_connect_view.h"

#include "lvgl.h"
#include "services/gui/widgets/spacer.h"
#include "services/gui/widgets/widgets.h"
#include "wifi_state.h"

#define TAG "wifi_connect_view"

// TODO: Standardize dialogs
void wifi_connect_view_create(WifiView* wifi_view, lv_obj_t* parent) {
    // TODO: Standardize this into "window content" function?
    // TODO: It can then be dynamically determined based on screen res and size
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(parent, 8, 0);
    lv_obj_set_style_pad_bottom(parent, 8, 0);
    lv_obj_set_style_pad_left(parent, 16, 0);
    lv_obj_set_style_pad_right(parent, 16, 0);

    WifiConnectView* connect_view = &wifi_view->connect_view;
    connect_view->root = parent;

    lv_obj_t* ssid_label = lv_label_create(parent);
    lv_label_set_text(ssid_label, "Network:");
    connect_view->ssid_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(connect_view->ssid_textarea, true);

    lv_obj_t* password_label = lv_label_create(parent);
    lv_label_set_text(password_label, "Password:");
    connect_view->password_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(connect_view->password_textarea, true);

    lv_obj_t* button_container = lv_obj_create(parent);
    lv_obj_set_width(button_container, LV_PCT(100));
    lv_obj_set_height(button_container, LV_SIZE_CONTENT);
    lv_obj_set_style_no_padding(button_container);
    lv_obj_set_style_border_width(button_container, 0, 0);
    lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);

    lv_obj_t* spacer_left = spacer(button_container, 1, 1);
    lv_obj_set_flex_grow(spacer_left, 1);

    connect_view->cancel_button = lv_btn_create(button_container);
    lv_obj_t* cancel_label = lv_label_create(connect_view->cancel_button);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
//    lv_obj_add_event_cb(connect_view->cancel_button, &sth, LV_EVENT_CLICKED, NULL);

    lv_obj_t* spacer_center = spacer(button_container, 4, 1);

    connect_view->connect_button = lv_btn_create(button_container);
    lv_obj_t* ok_label = lv_label_create(connect_view->connect_button);
    lv_label_set_text(ok_label, "Connect");
    lv_obj_center(ok_label);
    //    lv_obj_add_event_cb(connect_view->ok_button, &sth, LV_EVENT_CLICKED, NULL);
}

void wifi_connect_view_update(WifiView* view, WifiState* state) {
    lv_textarea_set_text(view->connect_view.ssid_textarea, (const char*)state->connect_ssid);
}
