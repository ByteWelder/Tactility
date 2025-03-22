#include "Tactility/app/serialconsole/ConnectView.h"
#include "Tactility/app/serialconsole/ConsoleView.h"

#include "Tactility/lvgl/Style.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/hal/uart/Uart.h>

#include <lvgl.h>

namespace tt::app::serialconsole {

class SerialConsoleApp final : public App {

private:

    lv_obj_t* disconnectButton = nullptr;
    lv_obj_t* wrapperWidget = nullptr;
    ConnectView connectView = ConnectView([this](auto uart){
        showConsoleView(std::move(uart));
    });
    ConsoleView consoleView;
    View* activeView = nullptr;

    void stopActiveView() {
        if (activeView != nullptr) {
            activeView->onStop();
            lv_obj_clean(wrapperWidget);
            activeView = nullptr;
        }
    }

    void showConsoleView(std::unique_ptr<hal::uart::Uart> uart) {
        stopActiveView();
        activeView = &consoleView;
        consoleView.onStart(wrapperWidget, std::move(uart));
        lv_obj_remove_flag(disconnectButton, LV_OBJ_FLAG_HIDDEN);
    }

    void showConnectView() {
        stopActiveView();
        activeView = &connectView;
        connectView.onStart(wrapperWidget);
        lv_obj_add_flag(disconnectButton, LV_OBJ_FLAG_HIDDEN);
    }

    void onDisconnect() {
        // Changing views (calling ConsoleView::stop()) also disconnects the UART
        showConnectView();
    }

    static void onDisconnectPressed(lv_event_t* event) {
        auto* app = (SerialConsoleApp*)lv_event_get_user_data(event);
        app->onDisconnect();
    }

public:

    SerialConsoleApp() = default;

    void onShow(AppContext& app, lv_obj_t* parent) final {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        auto* toolbar = lvgl::toolbar_create(parent, app);

        disconnectButton = lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_POWER, onDisconnectPressed, this);
        lv_obj_add_flag(disconnectButton, LV_OBJ_FLAG_HIDDEN);

        wrapperWidget = lv_obj_create(parent);
        lv_obj_set_width(wrapperWidget, LV_PCT(100));
        lv_obj_set_flex_grow(wrapperWidget, 1);
        lv_obj_set_flex_flow(wrapperWidget, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(wrapperWidget, 0, 0);
        lv_obj_set_style_border_width(wrapperWidget, 0, 0);
        lvgl::obj_set_style_bg_invisible(wrapperWidget);

        showConnectView();
    }

    void onHide(AppContext& app) final {
        stopActiveView();
    }
};

extern const AppManifest manifest = {
    .id = "SerialConsole",
    .name = "Serial Console",
    .icon = LV_SYMBOL_LIST,
    .type = Type::System,
    .createApp = create<SerialConsoleApp>
};

} // namespace
