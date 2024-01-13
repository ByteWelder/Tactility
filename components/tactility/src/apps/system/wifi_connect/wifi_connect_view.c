#include "wifi_connect_view.h"

#include "lvgl.h"
#include "ui/spacer.h"
#include "ui/style.h"
#include "wifi_connect.h"
#include "wifi_connect_bundle.h"
#include "wifi_connect_state.h"

static void on_connect(lv_event_t* event) {
    WifiConnect* wifi = (WifiConnect*)event->user_data;
    WifiConnectView* view = &wifi->view;
    const char* ssid = lv_textarea_get_text(view->ssid_textarea);
    const char* password = lv_textarea_get_text(view->password_textarea);

    WifiConnectBindings* bindings = &wifi->bindings;
    bindings->on_connect_ssid(
        ssid,
        password,
        bindings->on_connect_ssid_context
    );
}

// TODO: Standardize dialogs
void wifi_connect_view_create(App app, void* wifi, lv_obj_t* parent) {
    WifiConnect* wifi_connect = (WifiConnect*)wifi;
    WifiConnectView* view = &wifi_connect->view;
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
    lv_textarea_set_password_show_time(view->password_textarea, 0);
    lv_textarea_set_password_mode(view->password_textarea, true);

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
    lv_obj_add_event_cb(view->connect_button, &on_connect, LV_EVENT_CLICKED, wifi);

    // Init from app parameters
    Bundle* _Nullable bundle = tt_app_get_parameters(app);
    if (bundle) {
        char* ssid;
        if (tt_bundle_opt_string(bundle, WIFI_CONNECT_PARAM_SSID, &ssid)) {
            lv_textarea_set_text(view->ssid_textarea, ssid);
        }

        char* password;
        if (tt_bundle_opt_string(bundle, WIFI_CONNECT_PARAM_PASSWORD, &password)) {
            lv_textarea_set_text(view->password_textarea, password);
        }
    }
}

void wifi_connect_view_update(WifiConnectView* view, WifiConnectBindings* bindings, WifiConnectState* state) {
    UNUSED(view);
    UNUSED(bindings);
    UNUSED(state);
}
