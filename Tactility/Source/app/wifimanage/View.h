#pragma once

#include "app/AppContext.h"
#include "Bindings.h"
#include "State.h"
#include "lvgl.h"

namespace tt::app::wifimanage {

class View {

private:

    Bindings* bindings;
    State* state;
    lv_obj_t* root = nullptr;
    lv_obj_t* enable_switch = nullptr;
    lv_obj_t* enable_on_boot_switch = nullptr;
    lv_obj_t* scanning_spinner = nullptr;
    lv_obj_t* networks_list = nullptr;

    void updateWifiToggle();
    void updateEnableOnBootToggle();
    void updateScanning();
    void updateNetworkList();
    void createSsidListItem(const service::wifi::WifiApRecord& record, bool isConnecting);

public:

    View(Bindings* bindings, State* state) : bindings(bindings), state(state) {}

    void init(const AppContext& app, lv_obj_t* parent);
    void update();
};


} // namespace
