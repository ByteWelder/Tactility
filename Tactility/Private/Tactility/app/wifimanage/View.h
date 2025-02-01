#pragma once

#include "./Bindings.h"
#include "./State.h"

#include "Tactility/app/AppContext.h"

#include <lvgl.h>

namespace tt::app::wifimanage {

class View {

private:

    Bindings* bindings;
    State* state;
    std::unique_ptr<app::Paths> paths;
    lv_obj_t* root = nullptr;
    lv_obj_t* enable_switch = nullptr;
    lv_obj_t* enable_on_boot_switch = nullptr;
    lv_obj_t* scanning_spinner = nullptr;
    lv_obj_t* networks_list = nullptr;
    lv_obj_t* connect_to_hidden = nullptr;

    void updateWifiToggle();
    void updateEnableOnBootToggle();
    void updateScanning();
    void updateNetworkList();
    void updateConnectToHidden();
    void createSsidListItem(const service::wifi::ApRecord& record, bool isConnecting);

public:

    View(Bindings* bindings, State* state) : bindings(bindings), state(state) {}

    void init(const AppContext& app, lv_obj_t* parent);
    void update();
};


} // namespace
