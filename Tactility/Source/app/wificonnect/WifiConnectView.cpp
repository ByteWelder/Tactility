#include "WifiConnectView.h"

#include "Log.h"
#include "WifiConnect.h"
#include "WifiConnectBundle.h"
#include "WifiConnectState.h"
#include "WifiConnectStateUpdating.h"
#include "lvgl.h"
#include "service/gui/Gui.h"
#include "service/wifi/WifiSettings.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"

namespace tt::app::wificonnect {

#define TAG "wifi_connect"

static void view_set_loading(WifiConnectView* view, bool loading);

static void reset_errors(WifiConnectView* view) {
    lv_obj_add_flag(view->password_error, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->ssid_error, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->connection_error, LV_OBJ_FLAG_HIDDEN);
}

static void on_connect(lv_event_t* event) {
    WifiConnect* wifi = (WifiConnect*)lv_event_get_user_data(event);
    WifiConnectView* view = &wifi->view;

    state_set_radio_error(wifi, false);
    reset_errors(view);

    const char* ssid = lv_textarea_get_text(view->ssid_textarea);
    size_t ssid_len = strlen(ssid);
    if (ssid_len > TT_WIFI_SSID_LIMIT) {
        TT_LOG_E(TAG, "SSID too long");
        lv_label_set_text(view->ssid_error, "SSID too long");
        lv_obj_remove_flag(view->ssid_error, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const char* password = lv_textarea_get_text(view->password_textarea);
    size_t password_len = strlen(password);
    if (password_len > TT_WIFI_CREDENTIALS_PASSWORD_LIMIT) {
        TT_LOG_E(TAG, "Password too long");
        lv_label_set_text(view->password_error, "Password too long");
        lv_obj_remove_flag(view->password_error, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    bool store = lv_obj_get_state(view->remember_switch) & LV_STATE_CHECKED;

    view_set_loading(view, true);

    service::wifi::settings::WifiApSettings settings;
    strcpy((char*)settings.password, password);
    strcpy((char*)settings.ssid, ssid);
    settings.auto_connect = TT_WIFI_AUTO_CONNECT; // No UI yet, so use global setting:w

    WifiConnectBindings* bindings = &wifi->bindings;
    bindings->on_connect_ssid(
        &settings,
        store,
        bindings->on_connect_ssid_context
    );
}

static void view_set_loading(WifiConnectView* view, bool loading) {
    if (loading) {
        lv_obj_add_flag(view->connect_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(view->connecting_spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(view->password_textarea, LV_STATE_DISABLED);
        lv_obj_add_state(view->ssid_textarea, LV_STATE_DISABLED);
        lv_obj_add_state(view->remember_switch, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_flag(view->connect_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->connecting_spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(view->password_textarea, LV_STATE_DISABLED);
        lv_obj_remove_state(view->ssid_textarea, LV_STATE_DISABLED);
        lv_obj_remove_state(view->remember_switch, LV_STATE_DISABLED);
    }

}

void view_create_bottom_buttons(WifiConnect* wifi, lv_obj_t* parent) {
    WifiConnectView* view = &wifi->view;

    lv_obj_t* button_container = lv_obj_create(parent);
    lv_obj_set_width(button_container, LV_PCT(100));
    lv_obj_set_height(button_container, LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(button_container);
    lv_obj_set_style_border_width(button_container, 0, 0);

    view->remember_switch = lv_switch_create(button_container);
    lv_obj_add_state(view->remember_switch, LV_STATE_CHECKED);
    lv_obj_align(view->remember_switch, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* remember_label = lv_label_create(button_container);
    lv_label_set_text(remember_label, "Remember");
    lv_obj_align(remember_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align_to(remember_label, view->remember_switch, LV_ALIGN_OUT_RIGHT_MID, 4, 0);

    view->connecting_spinner = lv_spinner_create(button_container);
    lv_obj_set_size(view->connecting_spinner, 32, 32);
    lv_obj_align(view->connecting_spinner, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_flag(view->connecting_spinner, LV_OBJ_FLAG_HIDDEN);

    view->connect_button = lv_btn_create(button_container);
    lv_obj_t* connect_label = lv_label_create(view->connect_button);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_align(view->connect_button, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(view->connect_button, &on_connect, LV_EVENT_CLICKED, wifi);
}

// TODO: Standardize dialogs
void view_create(const App& app, void* wifi, lv_obj_t* parent) {
    WifiConnect* wifi_connect = (WifiConnect*)wifi;
    WifiConnectView* view = &wifi_connect->view;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    // SSID

    lv_obj_t* ssid_wrapper = lv_obj_create(wrapper);
    lv_obj_set_width(ssid_wrapper, LV_PCT(100));
    lv_obj_set_height(ssid_wrapper, LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(ssid_wrapper);
    lv_obj_set_style_border_width(ssid_wrapper, 0, 0);

    lv_obj_t* ssid_label_wrapper = lv_obj_create(ssid_wrapper);
    lv_obj_set_width(ssid_label_wrapper, LV_PCT(50));
    lv_obj_set_height(ssid_label_wrapper, LV_SIZE_CONTENT);
    lv_obj_align(ssid_label_wrapper, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_border_width(ssid_label_wrapper, 0, 0);
    lv_obj_set_style_pad_left(ssid_label_wrapper, 0, 0);
    lv_obj_set_style_pad_right(ssid_label_wrapper, 0, 0);

    lv_obj_t* ssid_label = lv_label_create(ssid_label_wrapper);
    lv_label_set_text(ssid_label, "Network:");

    view->ssid_textarea = lv_textarea_create(ssid_wrapper);
    lv_textarea_set_one_line(view->ssid_textarea, true);
    lv_obj_align(view->ssid_textarea, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_width(view->ssid_textarea, LV_PCT(50));

    view->ssid_error = lv_label_create(wrapper);
    lv_obj_set_style_text_color(view->ssid_error, lv_color_make(255, 50, 50), 0);
    lv_obj_add_flag(view->ssid_error, LV_OBJ_FLAG_HIDDEN);

    // Password

    lv_obj_t* password_wrapper = lv_obj_create(wrapper);
    lv_obj_set_width(password_wrapper, LV_PCT(100));
    lv_obj_set_height(password_wrapper, LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(password_wrapper);
    lv_obj_set_style_border_width(password_wrapper, 0, 0);

    lv_obj_t* password_label_wrapper = lv_obj_create(password_wrapper);
    lv_obj_set_width(password_label_wrapper, LV_PCT(50));
    lv_obj_set_height(password_label_wrapper, LV_SIZE_CONTENT);
    lv_obj_align_to(password_label_wrapper, password_wrapper, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_border_width(password_label_wrapper, 0, 0);
    lv_obj_set_style_pad_left(password_label_wrapper, 0, 0);
    lv_obj_set_style_pad_right(password_label_wrapper, 0, 0);

    lv_obj_t* password_label = lv_label_create(password_label_wrapper);
    lv_label_set_text(password_label, "Password:");

    view->password_textarea = lv_textarea_create(password_wrapper);
    lv_textarea_set_one_line(view->password_textarea, true);
    lv_textarea_set_password_mode(view->password_textarea, true);
    lv_obj_align(view->password_textarea, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_width(view->password_textarea, LV_PCT(50));

    view->password_error = lv_label_create(wrapper);
    lv_obj_set_style_text_color(view->password_error, lv_color_make(255, 50, 50), 0);
    lv_obj_add_flag(view->password_error, LV_OBJ_FLAG_HIDDEN);

    // Connection error
    view->connection_error = lv_label_create(wrapper);
    lv_obj_set_style_text_color(view->connection_error, lv_color_make(255, 50, 50), 0);
    lv_obj_add_flag(view->connection_error, LV_OBJ_FLAG_HIDDEN);

    // Bottom buttons
    view_create_bottom_buttons(wifi_connect, wrapper);

    // Keyboard bindings
    service::gui::keyboard_add_textarea(view->ssid_textarea);
    service::gui::keyboard_add_textarea(view->password_textarea);

    // Init from app parameters
    const Bundle& bundle = app.getParameters();
    std::string ssid;
    if (bundle.optString(WIFI_CONNECT_PARAM_SSID, ssid)) {
        lv_textarea_set_text(view->ssid_textarea, ssid.c_str());
    }

    std::string password;
    if (bundle.optString(WIFI_CONNECT_PARAM_PASSWORD, password)) {
        lv_textarea_set_text(view->password_textarea, password.c_str());
    }
}

void view_destroy(TT_UNUSED WifiConnectView* view) {
    // NO-OP
}

void view_update(
    WifiConnectView* view,
    TT_UNUSED WifiConnectBindings* bindings,
    WifiConnectState* state
) {
    if (state->connection_error) {
        view_set_loading(view, false);
        reset_errors(view);
        lv_label_set_text(view->connection_error, "Connection failed");
        lv_obj_remove_flag(view->connection_error, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace
