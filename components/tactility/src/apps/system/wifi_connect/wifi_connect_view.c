#include "wifi_connect_view.h"

#include "lvgl.h"
#include "ui/spacer.h"
#include "ui/style.h"
#include "wifi_connect_state.h"

#define TAG "wifi_connect_view"

static void on_connect(lv_event_t* event) {
    WifiConnectBindings* bindings = (WifiConnectBindings*)event->user_data;
    bindings->on_connect_ssid("On The Fence", "butallowedtoenter", bindings->on_connect_ssid_context);
}

// TODO: Standardize dialogs
void wifi_connect_view_create(WifiConnectView* view, WifiConnectBindings* bindings, lv_obj_t* parent) {
    // TODO: Standardize this into "window content" function?
    // TODO: It can then be dynamically determined based on screen res and size
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(parent, 8, 0);
    lv_obj_set_style_pad_bottom(parent, 8, 0);
    lv_obj_set_style_pad_left(parent, 16, 0);
    lv_obj_set_style_pad_right(parent, 16, 0);

    view->root = parent;

    lv_obj_t* ssid_label = lv_label_create(parent);
    lv_label_set_text(ssid_label, "Network:");
    view->ssid_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(view->ssid_textarea, true);

    lv_obj_t* password_label = lv_label_create(parent);
    lv_label_set_text(password_label, "Password:");
    view->password_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(view->password_textarea, true);

    lv_obj_t* button_container = lv_obj_create(parent);
    lv_obj_set_width(button_container, LV_PCT(100));
    lv_obj_set_height(button_container, LV_SIZE_CONTENT);
    tt_lv_obj_set_style_no_padding(button_container);
    lv_obj_set_style_border_width(button_container, 0, 0);
    lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);

    lv_obj_t* spacer_left = tt_lv_spacer_create(button_container, 1, 1);
    lv_obj_set_flex_grow(spacer_left, 1);

    view->connect_button = lv_btn_create(button_container);
    lv_obj_t* connect_label = lv_label_create(view->connect_button);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_center(connect_label);
    lv_obj_add_event_cb(view->connect_button, &on_connect, LV_EVENT_CLICKED, bindings);
}

void wifi_connect_view_update(WifiConnectView* view, WifiConnectBindings* bindings, WifiConnectState* state) {
    lv_textarea_set_text(view->ssid_textarea, (const char*)state->connect_ssid);
}
