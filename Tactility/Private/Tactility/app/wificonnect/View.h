#pragma once

#include "./Bindings.h"
#include "./State.h"

#include <Tactility/app/AppContext.h>

#include <lvgl.h>

namespace tt::app::wificonnect {

class WifiConnect;

class View final {

    Bindings* bindings;
    State* state;

public:

    lv_obj_t* ssid_textarea = nullptr;
    lv_obj_t* ssid_error = nullptr;
    lv_obj_t* password_textarea = nullptr;
    lv_obj_t* password_error = nullptr;
    lv_obj_t* connect_button = nullptr;
    lv_obj_t* remember_switch = nullptr;
    lv_obj_t* connecting_spinner = nullptr;
    lv_obj_t* connection_error = nullptr;
    lv_group_t* group = nullptr;

    View(Bindings* bindings, State* state) :
        bindings(bindings),
        state(state)
    {}

    void init(AppContext& app, lv_obj_t* parent);
    void update();

    void createBottomButtons(lv_obj_t* parent);
    void setLoading(bool loading);
    void resetErrors();
};


} // namespace
