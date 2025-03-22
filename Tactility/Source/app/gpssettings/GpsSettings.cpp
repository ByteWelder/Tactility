#include "Tactility/TactilityHeadless.h"
#include "Tactility/Timer.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/gps/GpsUtil.h"
#include "Tactility/service/loader/Loader.h"
#include <Tactility/service/gps/Gps.h>

#include <format>
#include <lvgl.h>

namespace tt::app::addgps {
extern AppManifest manifest;
}

namespace tt::app::gpssettings {

constexpr const char* TAG = "GpsSettings";

void createView(lv_obj_t* parent) {
    auto* wrapper = lv_obj_create(parent);
}

extern const AppManifest manifest;

class GpsSettingsApp final : public App {

private:

    std::unique_ptr<Timer> timer;
    std::shared_ptr<GpsSettingsApp*> appReference = std::make_shared<GpsSettingsApp*>(this);
    lv_obj_t* statusLabelWidget = nullptr;
    lv_obj_t* switchWidget = nullptr;
    lv_obj_t* spinnerWidget = nullptr;
    lv_obj_t* infoContainerWidget = nullptr;
    bool hasSetInfo = false;
    PubSub::SubscriptionHandle serviceStateSubscription = nullptr;

    static void onUpdateCallback(TT_UNUSED std::shared_ptr<void> context) {
        auto appPtr = std::static_pointer_cast<GpsSettingsApp*>(context);
        auto app = *appPtr;
        app->updateViews();
    }

    static void onServiceStateChangedCallback(const void* data, void* context) {
        auto* app = (GpsSettingsApp*)context;
        app->onServiceStateChanged();
    }

    void onServiceStateChanged() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (lock.lock(100 / portTICK_PERIOD_MS)) {
            if (!updateTimerState()) {
                updateViews();
            }
        }
    }

    static void onGpsToggledCallback(lv_event_t* event) {
        auto* app = (GpsSettingsApp*)lv_event_get_user_data(event);
        app->onGpsToggled(event);
    }

    static void onAddGpsCallback(lv_event_t* event) {
        auto* app = (GpsSettingsApp*)lv_event_get_user_data(event);
        app->onAddGps();
    }

    void onAddGps() {
        app::start(addgps::manifest.id);
    }

    void startReceivingUpdates() {
        timer->start(kernel::secondsToTicks(1));
        updateViews();
    }

    void stopReceivingUpdates() {
        timer->stop();
        updateViews();
    }

    void createInfoView(hal::gps::GpsModel model) {
        auto* label = lv_label_create(infoContainerWidget);
        lv_label_set_text_fmt(label, "Model: %s", toString(model));
    }

    void updateViews() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (lock.lock(100 / portTICK_PERIOD_MS)) {
            auto state = service::gps::getState();

            // Update toolbar
            switch (state) {
                case service::gps::State::OnPending:
                    TT_LOG_D(TAG, "OnPending");
                    lv_obj_remove_flag(spinnerWidget, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_state(switchWidget, LV_STATE_CHECKED);
                    lv_obj_add_state(switchWidget, LV_STATE_DISABLED);
                    break;
                case service::gps::State::On:
                    TT_LOG_D(TAG, "On");
                    lv_obj_add_flag(spinnerWidget, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_state(switchWidget, LV_STATE_CHECKED);
                    lv_obj_remove_state(switchWidget, LV_STATE_DISABLED);
                    break;
                case service::gps::State::OffPending:
                    TT_LOG_D(TAG, "OffPending");
                    lv_obj_remove_flag(spinnerWidget, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_state(switchWidget, LV_STATE_CHECKED);
                    lv_obj_add_state(switchWidget, LV_STATE_DISABLED);
                    break;
                case service::gps::State::Off:
                    TT_LOG_D(TAG, "Off");
                    lv_obj_add_flag(spinnerWidget, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_state(switchWidget, LV_STATE_CHECKED);
                    lv_obj_remove_state(switchWidget, LV_STATE_DISABLED);
                    break;
            }

            // Update status label and device info
            if (state == service::gps::State::On) {
                if (!hasSetInfo) {
                    auto devices = hal::findDevices<hal::gps::GpsDevice>(hal::Device::Type::Gps);
                    for (auto& device : devices) {
                        createInfoView(device->getModel());
                        hasSetInfo = true;
                    }
                }

                minmea_sentence_rmc rmc;
                if (service::gps::getCoordinates(rmc)) {
                    minmea_float latitude = { rmc.latitude.value, rmc.latitude.scale };
                    minmea_float longitude = { rmc.longitude.value, rmc.longitude.scale };
                    auto label_text = std::format("LAT {}\nLON {}", minmea_tocoord(&latitude), minmea_tocoord(&longitude));
                    lv_label_set_text(statusLabelWidget, label_text.c_str());
                } else {
                    lv_label_set_text(statusLabelWidget, "Acquiring lock...");
                }
                lv_obj_remove_flag(statusLabelWidget, LV_OBJ_FLAG_HIDDEN);
            } else {
                if (hasSetInfo) {
                    lv_obj_clean(infoContainerWidget);
                    hasSetInfo = false;
                }

                lv_obj_add_flag(statusLabelWidget, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    /** @return true if the views were updated */
    bool updateTimerState() {
        bool is_on = service::gps::getState() == service::gps::State::On;
        if (is_on && !timer->isRunning()) {
            startReceivingUpdates();
            return true;
        } else if (!is_on && timer->isRunning()) {
            stopReceivingUpdates();
            return true;
        } else {
            return false;
        }
    }

    void onGpsToggled(TT_UNUSED lv_event_t* event) {
        bool wants_on = lv_obj_has_state(switchWidget, LV_STATE_CHECKED);
        auto state = service::gps::getState();
        bool is_on = (state == service::gps::State::On) || (state == service::gps::State::OnPending);

        if (wants_on != is_on) {
            // start/stop are potentially blocking calls, so we use a dispatcher to not block the UI
            if (wants_on) {
                getMainDispatcher().dispatch([](TT_UNUSED auto _) {
                    service::gps::startReceiving();
                }, nullptr);
            } else {
                getMainDispatcher().dispatch([](TT_UNUSED auto _) {
                    service::gps::stopReceiving();
                }, nullptr);
            }
        }
    }

public:

    GpsSettingsApp() {
        timer = std::make_unique<Timer>(Timer::Type::Periodic, onUpdateCallback, appReference);
    }

    void onShow(AppContext& app, lv_obj_t* parent) final {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        auto* toolbar = lvgl::toolbar_create(parent, app);

        spinnerWidget = lvgl::toolbar_add_spinner_action(toolbar);
        lv_obj_add_flag(spinnerWidget, LV_OBJ_FLAG_HIDDEN);

        switchWidget = lvgl::toolbar_add_switch_action(toolbar);
        lv_obj_add_event_cb(switchWidget, onGpsToggledCallback, LV_EVENT_VALUE_CHANGED, this);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);
        lv_obj_set_style_border_width(main_wrapper, 0, 0);
        lv_obj_set_style_pad_all(main_wrapper, 0, 0);

        auto* top_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(top_wrapper, LV_PCT(100));
        lv_obj_set_height(top_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(top_wrapper, 0, 0);
        lv_obj_set_style_border_width(top_wrapper, 0, 0);

        statusLabelWidget = lv_label_create(top_wrapper);
        lv_obj_align(statusLabelWidget, LV_ALIGN_TOP_LEFT, 0, 0);

        infoContainerWidget = lv_obj_create(top_wrapper);
        lv_obj_align_to(infoContainerWidget, statusLabelWidget, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
        lv_obj_set_size(infoContainerWidget, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(infoContainerWidget, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_border_width(infoContainerWidget, 0, 0);
        lv_obj_set_style_pad_all(infoContainerWidget, 0, 0);
        hasSetInfo = false;

        updateTimerState();
        updateViews();

        serviceStateSubscription = service::gps::getStatePubsub()->subscribe(onServiceStateChangedCallback, this);

        auto* add_gps_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(add_gps_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(add_gps_wrapper, 0, 0);
        lv_obj_set_style_pad_all(add_gps_wrapper, 0, 0);

        auto* add_gps_button = lv_button_create(add_gps_wrapper);
        auto* add_gps_label = lv_label_create(add_gps_button);
        lv_label_set_text(add_gps_label, "Add GPS");
        lv_obj_add_event_cb(add_gps_button, onAddGpsCallback, LV_EVENT_SHORT_CLICKED, this);
        lv_obj_align(add_gps_button, LV_ALIGN_TOP_MID, 0, 0);
    }

    void onHide(AppContext& app) final {
        service::gps::getStatePubsub()->unsubscribe(serviceStateSubscription);
        serviceStateSubscription = nullptr;
    }
};

extern const AppManifest manifest = {
    .id = "GpsSettings",
    .name = "GPS",
    .icon = LV_SYMBOL_GPS,
    .type = Type::Settings,
    .createApp = create<GpsSettingsApp>
};

void start() {
    app::start(manifest.id);
}

} // namespace
