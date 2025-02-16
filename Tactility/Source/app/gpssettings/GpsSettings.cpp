#include "Tactility/Timer.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/gps/GpsUtil.h"
#include "Tactility/service/loader/Loader.h"
#include <Tactility/service/gps/Gps.h>

#include <format>
#include <ranges>
#include <lvgl.h>

#define TAG "text_viewer"

namespace tt::app::gpssettings {

extern const AppManifest manifest;

class GpsSettingsApp final : public App {

private:

    std::unique_ptr<Timer> timer;
    std::shared_ptr<GpsSettingsApp*> appReference = std::make_shared<GpsSettingsApp*>(this);
    lv_obj_t* statusLabelWidget = nullptr;
    lv_obj_t* switchWidget = nullptr;
    lv_obj_t* infoContainerWidget = nullptr;
    bool hasSetInfo = false;

    static void onUpdateCallback(std::shared_ptr<void> context) {
        auto appPtr = std::static_pointer_cast<GpsSettingsApp*>(context);
        auto app = *appPtr;
        app->updateViews();
    }

    static void onGpsToggledCallback(lv_event_t* event) {
        auto* app = (GpsSettingsApp*)lv_event_get_user_data(event);
        app->onGpsToggled(event);
    }

    void startReceivingUpdates() {
        timer->start(kernel::secondsToTicks(1));
        updateViews();
    }

    void stopReceivingUpdates() {
        timer->stop();
        updateViews();
    }

    void createInfoView(const hal::gps::GpsInfo& info) {
        auto* label = lv_label_create(infoContainerWidget);
        lv_label_set_text_fmt(
            label,
            "Module: %s\nFirmware version: %s\nProtocol version: %s",
            info.module.c_str(),
            info.firmwareVersion.c_str(),
            info.protocolVersion.c_str()
        );
    }

    void updateViews() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (lock.lock(100 / portTICK_PERIOD_MS)) {
            if (service::gps::isReceiving()) {
                if (!hasSetInfo) {
                    auto devices = hal::findDevices<hal::gps::GpsDevice>(hal::Device::Type::Gps);
                    auto devices_with_info = std::ranges::views::filter(devices, [](auto& device){
                      return device->getInfo().hasAnyData();
                    });

                    for (auto& device : devices_with_info) {
                        createInfoView(device->getInfo());
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

                lv_obj_add_state(switchWidget, LV_STATE_CHECKED);
            } else {
                if (hasSetInfo) {
                    lv_obj_clean(infoContainerWidget);
                    hasSetInfo = false;
                }

                lv_obj_add_flag(statusLabelWidget, LV_OBJ_FLAG_HIDDEN);

                lv_obj_remove_state(switchWidget, LV_STATE_CHECKED);
            }
        }
    }

    /** @return true if the views were updated */
    bool updateTimerState() {
        bool is_receiving = service::gps::isReceiving();
        if (is_receiving && !timer->isRunning()) {
            startReceivingUpdates();
            return true;
        } else if (!is_receiving && timer->isRunning()) {
            stopReceivingUpdates();
            return true;
        } else {
            return false;
        }
    }

    void onGpsToggled(lv_event_t* event) {
        bool wants_on = lv_obj_has_state(switchWidget, LV_STATE_CHECKED);
        bool is_on = service::gps::isReceiving();

        if (wants_on != is_on) {
            if (wants_on) {
                if (!service::gps::startReceiving()) {
                    TT_LOG_E(TAG, "Failed to toggle GPS on");
                }
            } else {
                service::gps::stopReceiving();
            }
        }

        if (!updateTimerState()) {
            updateViews();
        }
    }

public:

    GpsSettingsApp() {
        timer = std::make_unique<Timer>(Timer::Type::Periodic, onUpdateCallback, appReference);
    }

    void onShow(AppContext& app, lv_obj_t* parent) final {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        auto* toolbar = lvgl::toolbar_create(parent, app);
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
    }
};

extern const AppManifest manifest = {
    .id = "GpsSettings",
    .name = "GPS",
    .type = Type::Settings,
    .createApp = create<GpsSettingsApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
