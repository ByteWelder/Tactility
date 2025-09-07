#ifdef ESP_PLATFORM

#include "Tactility/lvgl/Lvgl.h"

#include <Tactility/Tactility.h>

#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/lvgl/Style.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/service/development/DevelopmentService.h>
#include <Tactility/service/development/DevelopmentSettings.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/Timer.h>

#include <cstring>
#include <lvgl.h>

namespace tt::app::development {

constexpr const char* TAG = "Development";

class DevelopmentApp final : public App {

    lv_obj_t* enableSwitch = nullptr;
    lv_obj_t* enableOnBootSwitch = nullptr;
    lv_obj_t* statusLabel = nullptr;
    std::shared_ptr<service::development::DevelopmentService> service;

    Timer timer = Timer(Timer::Type::Periodic, [this] {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (lock.lock(lvgl::defaultLockTime) && lvgl::isStarted()) {
            updateViewState();
        }
    });

    static void onEnableSwitchChanged(lv_event_t* event) {
        lv_event_code_t code = lv_event_get_code(event);
        auto* widget = static_cast<lv_obj_t*>(lv_event_get_target(event));
        if (code == LV_EVENT_VALUE_CHANGED) {
            bool is_on = lv_obj_has_state(widget, LV_STATE_CHECKED);
            auto* app = static_cast<DevelopmentApp*>(lv_event_get_user_data(event));
            bool is_changed = is_on != app->service->isEnabled();
            if (is_changed) {
                app->service->setEnabled(is_on);
            }
        }
    }

    static void onEnableOnBootSwitchChanged(lv_event_t* event) {
        lv_event_code_t code = lv_event_get_code(event);
        auto* widget = static_cast<lv_obj_t*>(lv_event_get_target(event));
        if (code == LV_EVENT_VALUE_CHANGED) {
            bool is_on = lv_obj_has_state(widget, LV_STATE_CHECKED);
            bool is_changed = is_on != service::development::shouldEnableOnBoot();
            if (is_changed) {
                // Dispatch it, so file IO doesn't block the UI
                getMainDispatcher().dispatch([is_on] {
                    service::development::setEnableOnBoot(is_on);
                });
            }
        }
    }

    void updateViewState() {
        if (!service->isEnabled()) {
            lv_label_set_text(statusLabel, "Service disabled");
        } else if (!service->isStarted()) {
            lv_label_set_text(statusLabel, "Waiting for connection...");
        } else { // enabled and started
            auto ip = service::wifi::getIp();
            if (ip.empty()) {
                lv_label_set_text(statusLabel, "Waiting for IP...");
            } else {
                const std::string status = std::string("Available at ") + ip;
                lv_label_set_text(statusLabel, status.c_str());
            }
        }
    }

public:

    void onCreate(AppContext& appContext) override {
        service = service::development::findService();
        if (service == nullptr) {
            TT_LOG_E(TAG, "Service not found");
            service::loader::stopApp();
        }
    }

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        // Toolbar
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);

        enableSwitch = lvgl::toolbar_add_switch_action(toolbar);
        lv_obj_add_event_cb(enableSwitch, onEnableSwitchChanged, LV_EVENT_VALUE_CHANGED, this);

        if (service->isEnabled()) {
            lv_obj_add_state(enableSwitch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(enableSwitch, LV_STATE_CHECKED);
        }

        // Wrappers

        lv_obj_t* secondary_flex = lv_obj_create(parent);
        lv_obj_set_width(secondary_flex, LV_PCT(100));
        lv_obj_set_flex_grow(secondary_flex, 1);
        lv_obj_set_flex_flow(secondary_flex, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_border_width(secondary_flex, 0, 0);
        lv_obj_set_style_pad_all(secondary_flex, 0, 0);
        lv_obj_set_style_pad_gap(secondary_flex, 0, 0);
        lvgl::obj_set_style_bg_invisible(secondary_flex);

        // align() methods don't work on flex, so we need this extra wrapper
        lv_obj_t* wrapper = lv_obj_create(secondary_flex);
        lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lvgl::obj_set_style_bg_invisible(wrapper);
        lv_obj_set_style_border_width(wrapper, 0, 0);

        // Enable on boot

        lv_obj_t* enable_label = lv_label_create(wrapper);
        lv_label_set_text(enable_label, "Enable on boot");
        lv_obj_align(enable_label, LV_ALIGN_TOP_LEFT, 0, 6);

        enableOnBootSwitch = lv_switch_create(wrapper);
        lv_obj_add_event_cb(enableOnBootSwitch, onEnableOnBootSwitchChanged, LV_EVENT_VALUE_CHANGED, this);
        lv_obj_align(enableOnBootSwitch, LV_ALIGN_TOP_RIGHT, 0, 0);
        if (service::development::shouldEnableOnBoot()) {
            lv_obj_add_state(enableOnBootSwitch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(enableOnBootSwitch, LV_STATE_CHECKED);
        }

        statusLabel = lv_label_create(wrapper);
        lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, 50);

        auto warning_label = lv_label_create(wrapper);
        lv_label_set_text(warning_label, "This feature is experimental and uses an unsecured http connection.");
        lv_obj_set_width(warning_label, LV_PCT(100));
        lv_label_set_long_mode(warning_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(warning_label, lv_color_make(0xff, 0xff, 00), LV_STATE_DEFAULT);
        lv_obj_align(warning_label, LV_ALIGN_TOP_LEFT, 0, 80);

        updateViewState();

        timer.start(1000);
    }

    void onHide(AppContext& appContext) override {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        // Ensure that the update isn't already happening
        lock.lock();
        timer.stop();
    }
};

extern const AppManifest manifest = {
    .id = "Development",
    .name = "Development",
    .type = Type::Settings,
    .createApp = create<DevelopmentApp>
};

void start() {
    app::start(manifest.id);
}

} // namespace

#endif // ESP_PLATFORM