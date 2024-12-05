#pragma once

#include "Bindings.h"
#include "State.h"

#include "app/AppContext.h"
#include "lvgl.h"

namespace tt::app::wificonnect {

class WifiConnect;

class View {

public:

    lv_obj_t* ssid_textarea = nullptr;
    lv_obj_t* ssid_error = nullptr;
    lv_obj_t* password_textarea = nullptr;
    lv_obj_t* password_error = nullptr;
    lv_obj_t* connect_button = nullptr;
    lv_obj_t* cancel_button = nullptr;
    lv_obj_t* remember_switch = nullptr;
    lv_obj_t* connecting_spinner = nullptr;
    lv_obj_t* connection_error = nullptr;
    lv_group_t* group = nullptr;

    void init(AppContext& app, WifiConnect* wifiConnect, lv_obj_t* parent);
    void update(Bindings* bindings, State* state);

    void createBottomButtons(WifiConnect* wifi, lv_obj_t* parent);
    void setLoading(bool loading);
    void resetErrors();
};


} // namespace
