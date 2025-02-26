#pragma once

#include "./View.h"

#include "Tactility/Tactility.h"
#include "Tactility/app/alertdialog/AlertDialog.h"
#include "Tactility/hal/uart/Uart.h"
#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/lvgl/Style.h"

#include <Tactility/StringUtils.h>
#include <array>
#include <string>

namespace tt::app::serialconsole {

class ConnectView final : public View {

public:

    typedef std::function<void(std::unique_ptr<hal::uart::Uart>)> OnConnectedFunction;
    std::vector<std::string> uartNames;

private:

    OnConnectedFunction onConnected;
    lv_obj_t* busDropdown = nullptr;
    lv_obj_t* speedTextarea = nullptr;

    void onConnect() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            return;
        }

        auto selected_uart_index = lv_dropdown_get_selected(busDropdown);
        if (selected_uart_index >= uartNames.size()) {
            alertdialog::start("Error", "No UART selected");
            return;
        }

        auto uart_name = uartNames[selected_uart_index];
        auto uart = hal::uart::open(uart_name);
        if (uart == nullptr) {
            alertdialog::start("Error", "Failed to connect to UART");
            return;
        }

        auto* speed_text = lv_textarea_get_text(speedTextarea);
        int speed = std::stoi(speed_text);
        if (speed == 0) {
            alertdialog::start("Error", "Invalid speed");
            return;
        }

        if (!uart->start()) {
            alertdialog::start("Error", "Failed to initialize");
            return;
        }

        if (!uart->setBaudRate(speed)) {
            uart->stop();
            alertdialog::start("Error", "Failed to set baud rate");
            return;
        }
    }

    static void onConnectCallback(lv_event_t* event) {
        auto* view = (ConnectView*)lv_event_get_user_data(event);
        view->onConnect();
    }

public:

    explicit ConnectView(OnConnectedFunction onConnected) : onConnected(std::move(onConnected)) {}

    void onStart(lv_obj_t* parent) final {
        for (auto& config : getConfiguration()->hardware->uart) {
            uartNames.push_back(config.name);
        }

        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_size(wrapper, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_ver(wrapper, 0, 0);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(wrapper);

        busDropdown = lv_dropdown_create(wrapper);

        auto bus_options = string::join(uartNames, "\n");
        lv_dropdown_set_options(busDropdown, bus_options.c_str());
        lv_obj_align(busDropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_width(busDropdown, LV_PCT(50));

        auto* bus_label = lv_label_create(wrapper);
        lv_obj_align(bus_label, LV_ALIGN_TOP_LEFT, 0, 10);
        lv_label_set_text(bus_label, "Bus");

        speedTextarea = lv_textarea_create(wrapper);
        lv_textarea_set_text(speedTextarea, "115200");
        lv_textarea_set_one_line(speedTextarea, true);
        lv_obj_set_width(speedTextarea, LV_PCT(50));
        lv_obj_align(speedTextarea, LV_ALIGN_TOP_RIGHT, 0, 40);

        auto* baud_rate_label = lv_label_create(wrapper);
        lv_obj_align(baud_rate_label, LV_ALIGN_TOP_LEFT, 0, 50);
        lv_label_set_text(baud_rate_label, "Baud");

        auto* connect_button = lv_button_create(wrapper);
        auto* connect_label = lv_label_create(connect_button);
        lv_label_set_text(connect_label, "Connect");
        lv_obj_align(connect_button, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_add_event_cb(connect_button, onConnectCallback, LV_EVENT_SHORT_CLICKED, this);
    }

    void onStop() final {
    }
};


} // namespace tt::app::serialconsole
