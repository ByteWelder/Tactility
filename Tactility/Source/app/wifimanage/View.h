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
    lv_obj_t* enable_on_boot_switch = nullptr;
    lv_obj_t* scanning_spinner = nullptr;
    lv_obj_t* networks_list = nullptr;
public:
    View() {}
    void init(const App& app, Bindings* bindings, lv_obj_t* parent);
    void update(Bindings* bindings, State* state);

private:

    void updateWifiToggle(State* state);
    void updateEnableOnBootToggle();
    void updateScanning(State* state);
    void updateNetworkList(State* state, Bindings* bindings);
    void createSsidListItem(Bindings* bindings, const service::wifi::WifiApRecord& record, bool isConnecting);
};


} // namespace
