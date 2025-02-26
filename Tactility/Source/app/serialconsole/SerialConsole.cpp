#include "Tactility/app/serialconsole/ConnectView.h"
#include "Tactility/app/serialconsole/ConsoleView.h"

#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/lvgl/Style.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/hal/uart/Uart.h>

#include <lvgl.h>

#define TAG "text_viewer"

namespace tt::app::serialconsole {

class SerialConsoleApp : public App {

private:

    enum class State {
        Initial,
        ShowConnectView,
        ShowConsoleView
    };

    lv_obj_t* disconnectButtonWidget = nullptr;
    lv_obj_t* wrapperWidget = nullptr;
    ConnectView connectView = ConnectView([this](auto uart){
    });
    ConsoleView consoleView;
    View* activeView = nullptr;
    std::unique_ptr<hal::uart::Uart> uart = nullptr;
    State state = State::Initial;

    void setState(State newState) {
        if (newState != state) {
            if (activeView != nullptr) {
                activeView->onStop();
            }

            lv_obj_clean(wrapperWidget);

            switch (newState) {
                case State::ShowConnectView:
                    activeView = &connectView;
                    break;
                case State::ShowConsoleView:
                    activeView = &consoleView;
                    break;
                default:
                    tt_crash("Illegal state");
            };
            assert(activeView != nullptr);
            state = newState;

            activeView->onStart(wrapperWidget);
        }
    }

    static void onDisconnectPressed(TT_UNUSED lv_event_t* event) {
    }

public:

    SerialConsoleApp() = default;

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        auto* toolbar = lvgl::toolbar_create(parent, app);
        disconnectButtonWidget = lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_POWER, onDisconnectPressed, this);
        lv_obj_add_flag(disconnectButtonWidget, LV_OBJ_FLAG_HIDDEN);

        wrapperWidget = lv_obj_create(parent);
        lv_obj_set_width(wrapperWidget, LV_PCT(100));
        lv_obj_set_flex_grow(wrapperWidget, 1);
        lv_obj_set_flex_flow(wrapperWidget, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(wrapperWidget, 0, 0);
        lv_obj_set_style_border_width(wrapperWidget, 0, 0);
        lvgl::obj_set_style_bg_invisible(wrapperWidget);

        setState(State::ShowConnectView);
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
