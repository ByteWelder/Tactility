#include <Tactility/app/wifimanage/View.h>

#include <Tactility/Tactility.h>
#include <Tactility/app/wifimanage/WifiManagePrivate.h>

#include <Tactility/lvgl/Style.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/lvgl/Spinner.h>

#include <Tactility/Log.h>
#include <Tactility/service/wifi/Wifi.h>

#include <format>
#include <string>
#include <set>
#include <Tactility/service/wifi/WifiSettings.h>

namespace tt::app::wifimanage {

constexpr auto* TAG = "WifiManageView";

std::shared_ptr<WifiManage> _Nullable optWifiManage();

uint8_t mapRssiToPercentage(int rssi) {
    auto abs_rssi = std::abs(rssi);
    if (abs_rssi < 30U) {
        abs_rssi = 30U;
    } else if (abs_rssi > 90U) {
        abs_rssi = 90U;
    }

    auto percentage = (float)(90U - abs_rssi) / 60.f * 100.f;
    return (uint8_t)percentage;
}

static void on_enable_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);

        auto wifi = std::static_pointer_cast<WifiManage>(getCurrentApp());
        auto bindings = wifi->getBindings();

        bindings.onWifiToggled(is_on);
    }
}

static void on_enable_on_boot_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        // Dispatch it, so file IO doesn't block the UI
        getMainDispatcher().dispatch([is_on] {
            service::wifi::settings::setEnableOnBoot(is_on);
        });
    }
}

static void onConnectToHiddenClicked(lv_event_t* event) {
    auto* bindings = (Bindings*)lv_event_get_user_data(event);
    bindings->onConnectToHidden();
}

// region Secondary updates

static void connect(lv_event_t* event) {
    auto* wrapper = lv_event_get_current_target_obj(event);
    // Assumes that the second child of the button is a label ... risky
    auto* label = lv_obj_get_child(wrapper, 0);
    // We get the SSID from the button label because it's safer than alloc'ing
    // our own and passing it as the event data
    const char* ssid = lv_label_get_text(label);
    if (ssid != nullptr) {
        TT_LOG_I(TAG, "Clicked AP: %s", ssid);
        auto* bindings = (Bindings*)lv_event_get_user_data(event);

        std::string connection_target = service::wifi::getConnectionTarget();
        if (connection_target == ssid) {
            bindings->onDisconnect();
        } else {
            bindings->onConnectSsid(ssid);
        }
    }
}

static void showDetails(lv_event_t* event) {
    auto* wrapper = lv_event_get_current_target_obj(event);
    // Hack: Get the hidden label with the ssid
    auto* ssid_label = lv_obj_get_child(wrapper, 1);
    const char* ssid = lv_label_get_text(ssid_label);
    auto* bindings = (Bindings*)lv_event_get_user_data(event);
    bindings->onShowApSettings(ssid);
}

void View::createSsidListItem(const service::wifi::ApRecord& record, bool isConnecting) {
    auto ui_scale = hal::getConfiguration()->uiScale;

    auto* wrapper = lv_obj_create(networks_list);
    lv_obj_add_event_cb(wrapper, &connect, LV_EVENT_SHORT_CLICKED, bindings);
    lv_obj_set_user_data(wrapper, bindings);
    lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_margin_all(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);

    auto* label = lv_label_create(wrapper);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label, record.ssid.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, LV_PCT(70));

    auto* info_wrapper = lv_obj_create(wrapper);
    lv_obj_set_style_margin_all(info_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_align(info_wrapper, LV_ALIGN_RIGHT_MID);

    if (ui_scale == hal::UiScale::Smallest) {
        lv_obj_set_size(info_wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(info_wrapper, 4, LV_STATE_DEFAULT);
    } else {
        lv_obj_set_size(info_wrapper, 36, 36);
        lv_obj_set_style_pad_all(info_wrapper, 0, LV_STATE_DEFAULT);
    }

    lv_obj_set_style_border_color(info_wrapper, lv_theme_get_color_primary(info_wrapper), 0);
    lv_obj_add_event_cb(info_wrapper, &showDetails, LV_EVENT_SHORT_CLICKED, bindings);
    lv_obj_align(info_wrapper, LV_ALIGN_RIGHT_MID, 0, 0);

    auto* info_label = lv_label_create(info_wrapper);
    lv_label_set_text(info_label, "i");
    // Hack: Create a hidden label to store data and pass it to the callback
    auto* ssid_label = lv_label_create(info_wrapper);
    lv_label_set_text(ssid_label, record.ssid.c_str());
    lv_obj_add_flag(ssid_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(info_label, lv_theme_get_color_primary(info_wrapper), LV_STATE_DEFAULT);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    if (isConnecting) {
        auto* connecting_spinner = lvgl::spinner_create(wrapper);
        auto spinner_offset_x = (ui_scale == hal::UiScale::Smallest) ? -2 : -8;
        lv_obj_align_to(connecting_spinner, info_wrapper, LV_ALIGN_OUT_LEFT_MID, spinner_offset_x, 0);
    } else {
        auto percentage = mapRssiToPercentage(record.rssi);

        std::string auth_info;
        if (record.auth_mode == WIFI_AUTH_OPEN) {
            auth_info = "(open)";
        } else {
            auth_info = "";
        }

        auto signal = std::format("{}{}%", auth_info, percentage);
        auto* signal_label = lv_label_create(wrapper);
        lv_label_set_text(signal_label, signal.c_str());
        auto info_label_offset = (ui_scale == hal::UiScale::Smallest) ? -4 : -16;
        lv_obj_align_to(signal_label, info_wrapper, LV_ALIGN_OUT_LEFT_MID, info_label_offset, 0);
    }
}

void View::updateConnectToHidden() {
    using enum service::wifi::RadioState;
    switch (state->getRadioState()) {
        case On:
        case ConnectionPending:
        case ConnectionActive:
            lv_obj_remove_flag(connect_to_hidden, LV_OBJ_FLAG_HIDDEN);
            break;

        case OnPending:
        case OffPending:
        case Off:
            lv_obj_add_flag(connect_to_hidden, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

void View::updateNetworkList() {
    lv_obj_clean(networks_list);

    switch (state->getRadioState()) {
        using enum service::wifi::RadioState;
        case OnPending:
        case On:
        case ConnectionPending:
        case ConnectionActive: {

            std::string connection_target = service::wifi::getConnectionTarget();

            state->withApRecords([this, &connection_target](const std::vector<service::wifi::ApRecord>& apRecords){
                bool is_connected = !connection_target.empty() &&
                    state->getRadioState() == ConnectionActive;
                bool added_connected = false;
                if (is_connected && !apRecords.empty()) {
                    for (auto &record : apRecords) {
                        if (record.ssid == connection_target) {
                            lv_list_add_text(networks_list, "Connected");
                            createSsidListItem(record, false);
                            added_connected = true;
                            break;
                        }
                    }
                }

                lv_list_add_text(networks_list, "Other networks");
                std::set<std::string> used_ssids;
                if (!apRecords.empty()) {
                    for (auto& record : apRecords) {
                        if (used_ssids.find(record.ssid) == used_ssids.end()) {
                            bool connection_target_match = (record.ssid == connection_target);
                            bool is_connecting = connection_target_match
                                && state->getRadioState() == ConnectionPending &&
                                !connection_target.empty();
                            bool skip = connection_target_match && added_connected;
                            if (!skip) {
                                createSsidListItem(record, is_connecting);
                            }
                            used_ssids.insert(record.ssid);
                        }
                    }
                    lv_obj_clear_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
                } else if (!state->hasScannedAfterRadioOn() || state->isScanning()) {
                    // hasScannedAfterRadioOn() prevents briefly showing "No networks found" when turning radio on.
                    lv_obj_add_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_clear_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_t* label = lv_label_create(networks_list);
                    lv_label_set_text(label, "No networks found.");
                }
            });

            break;
        }
        case OffPending:
        case Off: {
            lv_obj_add_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }
}

void View::updateScanning() {
    if (state->getRadioState() == service::wifi::RadioState::On && state->isScanning()) {
        lv_obj_clear_flag(scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void View::updateWifiToggle() {
    lv_obj_clear_state(enable_switch, LV_STATE_ANY);
    switch (state->getRadioState()) {
        using enum service::wifi::RadioState;
        case On:
        case ConnectionPending:
        case ConnectionActive:
            lv_obj_add_state(enable_switch, LV_STATE_CHECKED);
            break;
        case OnPending:
            lv_obj_add_state(enable_switch, LV_STATE_CHECKED | LV_STATE_DISABLED);
            break;
        case Off:
            lv_obj_remove_state(enable_switch, LV_STATE_CHECKED | LV_STATE_DISABLED);
            break;
        case OffPending:
            lv_obj_remove_state(enable_switch, LV_STATE_CHECKED);
            lv_obj_add_state(enable_switch, LV_STATE_DISABLED);
            break;
    }
}

void View::updateEnableOnBootToggle() {
    lv_obj_clear_state(enable_on_boot_switch, LV_STATE_ANY);
    if (service::wifi::settings::shouldEnableOnBoot()) {
        lv_obj_add_state(enable_on_boot_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(enable_on_boot_switch, LV_STATE_CHECKED);
    }
}

// endregion Secondary updates

// region Main

void View::init(const AppContext& app, lv_obj_t* parent) {
    auto ui_scale = hal::getConfiguration()->uiScale;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    root = parent;

    paths = app.getPaths();

    // Toolbar

    lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);

    scanning_spinner = lvgl::toolbar_add_spinner_action(toolbar);

    enable_switch = lvgl::toolbar_add_switch_action(toolbar);
    lv_obj_add_event_cb(enable_switch, on_enable_switch_changed, LV_EVENT_VALUE_CHANGED, bindings);

    // Wrappers

    auto* flex_wrapper = lv_obj_create(parent);
    lv_obj_set_width(flex_wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(flex_wrapper, 1);
    lv_obj_set_flex_flow(flex_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(flex_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(flex_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(flex_wrapper, 0, LV_STATE_DEFAULT);
    lvgl::obj_set_style_bg_invisible(flex_wrapper);

    // Fixed size content wrapper: align() methods don't work on flex, so we need this extra wrapper

    auto* content_wrapper = lv_obj_create(flex_wrapper);
    lv_obj_set_size(content_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lvgl::obj_set_style_bg_invisible(content_wrapper);
    lv_obj_set_style_border_width(content_wrapper, 0, LV_STATE_DEFAULT);

    // Enable on boot

    auto* enable_on_boot_wrapper = lv_obj_create(content_wrapper);
    lv_obj_set_size(enable_on_boot_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(enable_on_boot_wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(enable_on_boot_wrapper, 0, LV_STATE_DEFAULT);

    auto* enable_label = lv_label_create(enable_on_boot_wrapper);
    lv_label_set_text(enable_label, "Enable on boot");
    lv_obj_align(enable_label, LV_ALIGN_LEFT_MID, 0, 0);

    enable_on_boot_switch = lv_switch_create(enable_on_boot_wrapper);
    lv_obj_add_event_cb(enable_on_boot_switch, on_enable_on_boot_switch_changed, LV_EVENT_VALUE_CHANGED, bindings);
    lv_obj_align(enable_on_boot_switch, LV_ALIGN_RIGHT_MID, 0, 0);

    // Networks

    networks_list = lv_obj_create(content_wrapper);
    lv_obj_set_flex_flow(networks_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(networks_list, LV_PCT(100));
    lv_obj_set_height(networks_list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(networks_list, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(networks_list, 0, LV_STATE_DEFAULT);
    const int network_list_y_offset = ui_scale == hal::UiScale::Smallest ? 22 : 44;
    lv_obj_align(networks_list, LV_ALIGN_TOP_LEFT, 0, network_list_y_offset);

    connect_to_hidden = lv_button_create(flex_wrapper);
    lv_obj_set_width(connect_to_hidden, LV_PCT(100));
    lv_obj_set_style_margin_bottom(connect_to_hidden, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_margin_hor(connect_to_hidden, 12, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(connect_to_hidden, onConnectToHiddenClicked, LV_EVENT_SHORT_CLICKED, bindings);
    auto* connect_to_hidden_label = lv_label_create(connect_to_hidden);
    lv_label_set_text(connect_to_hidden_label, "Connect to hidden SSID");
}

void View::update() {
    updateWifiToggle();
    updateEnableOnBootToggle();
    updateScanning();
    updateNetworkList();
    updateConnectToHidden();
}

} // namespace
