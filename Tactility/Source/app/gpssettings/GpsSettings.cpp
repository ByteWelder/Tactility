#include "Tactility/app/AppManifest.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/loader/Loader.h"
#include <Tactility/service/gps/Gps.h>

#include <lvgl.h>

#define TAG "text_viewer"

namespace tt::app::gpssettings {

extern const AppManifest manifest;

class GpsSettingsApp final : public App {

private:

    static void onGpsToggled(lv_event_t* event) {
        auto* widget = lv_event_get_target_obj(event);

        bool wants_on = lv_obj_has_state(widget, LV_STATE_CHECKED);
        bool is_on = service::gps::isReceiving();

        if (wants_on != is_on) {
            if (wants_on) {
                if (!service::gps::startReceiving()) {
                    TT_LOG_E(TAG, "Failed to toggle GPS on");
                    lv_obj_remove_state(widget, LV_STATE_CHECKED);
                }
            } else {
                service::gps::stopReceiving();
            }
        }
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) final {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        auto* top_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(top_wrapper, LV_PCT(100));
        lv_obj_set_height(top_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(top_wrapper, 0, 0);
        lv_obj_set_style_border_width(top_wrapper, 0, 0);

        auto* toggle_label = lv_label_create(top_wrapper);
        lv_label_set_text(toggle_label, "GPS receiver");

        auto* toggle_switch = lv_switch_create(top_wrapper);
        lv_obj_align(toggle_switch, LV_ALIGN_TOP_RIGHT, 0, 0);

        if (service::gps::isReceiving()) {
            lv_obj_add_state(toggle_switch, LV_STATE_CHECKED);
        }

        lv_obj_add_event_cb(toggle_switch, onGpsToggled, LV_EVENT_VALUE_CHANGED, nullptr);
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
