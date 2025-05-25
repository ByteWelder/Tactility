#include "Tactility/app/timezone/TimeZone.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/LvglSync.h"

#include <Tactility/Assets.h>
#include <Tactility/time/Time.h>
#include <lvgl.h>

#define TAG "text_viewer"

namespace tt::app::timedatesettings {

extern const AppManifest manifest;

class TimeDateSettingsApp : public App {

private:

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    lv_obj_t* regionLabelWidget = nullptr;

    static void onConfigureTimeZonePressed(TT_UNUSED lv_event_t* event) {
        timezone::start();
    }

    static void onTimeFormatChanged(lv_event_t* event) {
        auto* widget = lv_event_get_target_obj(event);
        bool show_24 = lv_obj_has_state(widget, LV_STATE_CHECKED);
        time::setTimeFormat24Hour(show_24);
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        auto* region_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(region_wrapper, LV_PCT(100));
        lv_obj_set_height(region_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(region_wrapper, 0, 0);
        lv_obj_set_style_border_width(region_wrapper, 0, 0);

        auto* region_prefix_label = lv_label_create(region_wrapper);
        lv_label_set_text(region_prefix_label, "Region: ");
        lv_obj_align(region_prefix_label, LV_ALIGN_LEFT_MID, 0, 0);

        auto* region_label = lv_label_create(region_wrapper);
        std::string timeZoneName = time::getTimeZoneName();
        if (timeZoneName.empty()) {
            timeZoneName = "not set";
        }
        regionLabelWidget = region_label;
        lv_label_set_text(region_label, timeZoneName.c_str());
        // TODO: Find out why Y offset is needed
        lv_obj_align_to(region_label, region_prefix_label, LV_ALIGN_OUT_RIGHT_MID, 0, 8);

        auto* region_button = lv_button_create(region_wrapper);
        lv_obj_align(region_button, LV_ALIGN_TOP_RIGHT, 0, 0);
        auto* region_button_image = lv_image_create(region_button);
        lv_obj_add_event_cb(region_button, onConfigureTimeZonePressed, LV_EVENT_SHORT_CLICKED, nullptr);
        lv_image_set_src(region_button_image, LV_SYMBOL_SETTINGS);

        auto* time_format_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(time_format_wrapper, LV_PCT(100));
        lv_obj_set_height(time_format_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(time_format_wrapper, 0, 0);
        lv_obj_set_style_border_width(time_format_wrapper, 0, 0);

        auto* time_24h_label = lv_label_create(time_format_wrapper);
        lv_label_set_text(time_24h_label, "24-hour clock");
        lv_obj_align(time_24h_label, LV_ALIGN_LEFT_MID, 0, 0);

        auto* time_24h_switch = lv_switch_create(time_format_wrapper);
        lv_obj_align(time_24h_switch, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(time_24h_switch, onTimeFormatChanged, LV_EVENT_VALUE_CHANGED, nullptr);
        if (time::isTimeFormat24Hour()) {
            lv_obj_add_state(time_24h_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(time_24h_switch, LV_STATE_CHECKED);
        }
    }

    void onResult(AppContext& app, TT_UNUSED LaunchId launchId, Result result, std::unique_ptr<Bundle> bundle) override {
        if (result == Result::Ok && bundle != nullptr) {
            auto name = timezone::getResultName(*bundle);
            auto code = timezone::getResultCode(*bundle);
            TT_LOG_I(TAG, "Result name=%s code=%s", name.c_str(), code.c_str());
            time::setTimeZone(name, code);

            if (!name.empty()) {
                if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
                    lv_label_set_text(regionLabelWidget, name.c_str());
                    lvgl::unlock();
                }
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "TimeDateSettings",
    .name = "Time & Date",
    .icon = TT_ASSETS_APP_ICON_TIME_DATE_SETTINGS,
    .type = Type::Settings,
    .createApp = create<TimeDateSettingsApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
