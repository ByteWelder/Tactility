#pragma once

#include "app/App.h"
#include "Bindings.h"
#include "State.h"
#include "lvgl.h"

namespace tt::app::wifimanage {

class View {
private:
    lv_obj_t* root = nullptr;
    lv_obj_t* enable_switch = nullptr;
    lv_obj_t* scanning_spinner = nullptr;
    lv_obj_t* networks_label = nullptr;
    lv_obj_t* networks_list = nullptr;
    lv_obj_t* connected_ap_container = nullptr;
    lv_obj_t* connected_ap_label = nullptr;
public:
    View() {}
    void init(const App& app, Bindings* bindings, lv_obj_t* parent);
    void update(Bindings* bindings, State* state);

private:

    void updateConnectedAp(State* state, TT_UNUSED Bindings* bindings);
    void updateWifiToggle(State* state);
    void updateScanning(State* state);
    void updateNetworkList(State* state, Bindings* bindings);
    void createNetworkButton(Bindings* bindings, const service::wifi::WifiApRecord& record);
};


} // namespace
