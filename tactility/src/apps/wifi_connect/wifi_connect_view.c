#include "wifi_connect_view.h"

#include "log.h"
#include "lvgl.h"
#include "services/gui/gui.h"
#include "services/wifi/wifi_credentials.h"
#include "ui/spacer.h"
#include "ui/style.h"
#include "ui/toolbar.h"
#include "wifi_connect.h"
#include "wifi_connect_bundle.h"
#include "wifi_connect_state.h"

#define TAG "wifi_connect"

static void on_connect(lv_event_t* event) {
    WifiConnect* wifi = (WifiConnect*)lv_event_get_user_data(event);
    WifiConnectView* view = &wifi->view;
    const char* ssid = lv_textarea_get_text(view->ssid_textarea);
    const char* password = lv_textarea_get_text(view->password_textarea);

    if (strlen(password) > 63) {
        // TODO: UI feedback
        TT_LOG_E(TAG, "Password too long");
        return;
    }
    char password_buffer[64];
    strcpy(password_buffer, password);

    WifiConnectBindings* bindings = &wifi->bindings;
    bindings->on_connect_ssid(
        ssid,
        password_buffer,
        bindings->on_connect_ssid_context
    );

    if (lv_obj_get_state(view->remember_switch) == LV_STATE_CHECKED) {
        tt_wifi_credentials_set(ssid, password_buffer);
    }
}

void wifi_connect_view_create_bottom_buttons(WifiConnect* wifi, lv_obj_t* parent) {
    WifiConnectView* view = &wifi->view;

    lv_obj_t* button_container = lv_obj_create(parent);
    lv_obj_set_width(button_container, LV_PCT(100));
    lv_obj_set_height(button_container, LV_SIZE_CONTENT);
    tt_lv_obj_set_style_no_padding(button_container);
    lv_obj_set_style_border_width(button_container, 0, 0);

    view->remember_switch = lv_switch_create(button_container);
    lv_obj_add_state(view->remember_switch, LV_STATE_CHECKED);
    lv_obj_align(view->remember_switch, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* remember_label = lv_label_create(button_container);
    lv_label_set_text(remember_label, "Remember");
    lv_obj_align(remember_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align_to(remember_label, view->remember_switch, LV_ALIGN_OUT_RIGHT_MID, 4, 0);

    view->connect_button = lv_btn_create(button_container);
    lv_obj_t* connect_label = lv_label_create(view->connect_button);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_align(view->connect_button, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(view->connect_button, &on_connect, LV_EVENT_CLICKED, wifi);
}

// TODO: Standardize dialogs
void wifi_connect_view_create(App app, void* wifi, lv_obj_t* parent) {
    WifiConnect* wifi_connect = (WifiConnect*)wifi;
    WifiConnectView* view = &wifi_connect->view;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* ssid_label = lv_label_create(wrapper);
    lv_label_set_text(ssid_label, "Network:");
    view->ssid_textarea = lv_textarea_create(wrapper);
    lv_textarea_set_one_line(view->ssid_textarea, true);

    tt_lv_spacer_create(wrapper, 1, 8);

    lv_obj_t* password_label = lv_label_create(wrapper);
    lv_label_set_text(password_label, "Password:");
    view->password_textarea = lv_textarea_create(wrapper);
    lv_textarea_set_one_line(view->password_textarea, true);
    lv_textarea_set_password_mode(view->password_textarea, true);

    tt_lv_spacer_create(wrapper, 1, 8);

    wifi_connect_view_create_bottom_buttons(wifi, wrapper);

    gui_keyboard_add_textarea(view->ssid_textarea);
    gui_keyboard_add_textarea(view->password_textarea);

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

void wifi_connect_view_destroy(TT_UNUSED WifiConnectView* view) {
    // NO-OP
}

void wifi_connect_view_update(
    TT_UNUSED WifiConnectView* view,
    TT_UNUSED WifiConnectBindings* bindings,
    TT_UNUSED WifiConnectState* state
) {
    // NO-OP
}
