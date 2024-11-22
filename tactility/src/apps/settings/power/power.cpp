#include "app.h"
#include "assets.h"
#include "lvgl.h"
#include "tactility.h"
#include "timer.h"
#include "ui/lvgl_sync.h"
#include "ui/style.h"
#include "ui/toolbar.h"

namespace tt::app::settings::power {

#define TAG "power"

typedef struct {
    Timer* update_timer;
    const hal::Power* power;
    lv_obj_t* enable_switch;
    lv_obj_t* charge_state;
    lv_obj_t* charge_level;
    lv_obj_t* current;
} AppData;

static void app_update_ui(App app) {
    auto* data = static_cast<AppData*>(tt_app_get_data(app));

    bool charging_enabled = data->power->is_charging_enabled();
    const char* charge_state = data->power->is_charging() ? "yes" : "no";
    uint8_t charge_level = data->power->get_charge_level();
    uint16_t charge_level_scaled = (int16_t)charge_level * 100 / 255;
    int32_t current = data->power->get_current();

    lvgl::lock(ms_to_ticks(1000));
    lv_obj_set_state(data->enable_switch, LV_STATE_CHECKED, charging_enabled);
    lv_label_set_text_fmt(data->charge_state, "Charging: %s", charge_state);
    lv_label_set_text_fmt(data->charge_level, "Charge level: %d%%", charge_level_scaled);
#ifdef ESP_PLATFORM
    lv_label_set_text_fmt(data->current, "Current: %ld mAh", current);
#else
    lv_label_set_text_fmt(data->current, "Current: %d mAh", current);
#endif
    lvgl::unlock();
}

static void on_power_enabled_change(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        App app = lv_event_get_user_data(event);
        auto* data = static_cast<AppData*>(tt_app_get_data(app));
        if (data->power->is_charging_enabled() != is_on) {
            data->power->set_charging_enabled(is_on);
            app_update_ui(app);
        }
    }
}

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    auto* data = static_cast<AppData*>(tt_app_get_data(app));

    // Top row: enable/disable
    lv_obj_t* switch_container = lv_obj_create(wrapper);
    lv_obj_set_width(switch_container, LV_PCT(100));
    lv_obj_set_height(switch_container, LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(switch_container);
    lvgl::obj_set_style_bg_invisible(switch_container);

    lv_obj_t* enable_label = lv_label_create(switch_container);
    lv_label_set_text(enable_label, "Charging enabled");
    lv_obj_set_align(enable_label, LV_ALIGN_LEFT_MID);

    lv_obj_t* enable_switch = lv_switch_create(switch_container);
    lv_obj_add_event_cb(enable_switch, on_power_enabled_change, LV_EVENT_ALL, app);
    lv_obj_set_align(enable_switch, LV_ALIGN_RIGHT_MID);

    data->enable_switch = enable_switch;
    data->charge_state = lv_label_create(wrapper);
    data->charge_level = lv_label_create(wrapper);
    data->current = lv_label_create(wrapper);

    app_update_ui(app);
    timer_start(data->update_timer, ms_to_ticks(1000));
}

static void app_hide(TT_UNUSED App app) {
    auto* data = static_cast<AppData*>(tt_app_get_data(app));
    timer_stop(data->update_timer);
}

static void app_start(App app) {
    auto* data = static_cast<AppData*>(malloc(sizeof(AppData)));
    tt_app_set_data(app, data);
    data->update_timer = timer_alloc(&app_update_ui, TimerTypePeriodic, app);
    data->power = get_config()->hardware->power;
    assert(data->power != nullptr); // The Power app only shows up on supported devices
}

static void app_stop(App app) {
    auto* data = static_cast<AppData*>(tt_app_get_data(app));
    timer_free(data->update_timer);
    free(data);
}

extern const AppManifest manifest = {
    .id = "power",
    .name = "Power",
    .icon = TT_ASSETS_APP_ICON_POWER_SETTINGS,
    .type = AppTypeSettings,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};

} // namespace
