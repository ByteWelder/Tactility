#include <Tactility/Assets.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/Time.h>

#include <lvgl.h>

namespace tt::app::timedatesettings {

constexpr auto* TAG = "TimeDate";

extern const AppManifest manifest;

class TimeDateSettingsApp : public App {

    Mutex mutex = Mutex(Mutex::Type::Recursive);

    static void onTimeFormatChanged(lv_event_t* event) {
        auto* widget = lv_event_get_target_obj(event);
        bool show_24 = lv_obj_has_state(widget, LV_STATE_CHECKED);
        settings::setTimeFormat24Hour(show_24);
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

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
        if (settings::isTimeFormat24Hour()) {
            lv_obj_add_state(time_24h_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(time_24h_switch, LV_STATE_CHECKED);
        }
    }
};

extern const AppManifest manifest = {
    .id = "TimeDateSettings",
    .name = "Time & Date",
    .icon = TT_ASSETS_APP_ICON_TIME_DATE_SETTINGS,
    .category = Category::Settings,
    .createApp = create<TimeDateSettingsApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
