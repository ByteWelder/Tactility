#include "app/AppContext.h"
#include "Assets.h"
#include "lvgl.h"
#include "Tactility.h"
#include "Timer.h"
#include "lvgl/LvglSync.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"

namespace tt::app::power {

#define TAG "power"

typedef struct {
    Timer* update_timer;
    const hal::Power* power;
    lv_obj_t* enable_switch;
    lv_obj_t* charge_state;
    lv_obj_t* charge_level;
    lv_obj_t* current;
} Data;

static void updateUi(void* callbackContext) {
    auto* data = (Data*)callbackContext;
    bool charging_enabled = data->power->isChargingEnabled();
    const char* charge_state = data->power->isCharging() ? "yes" : "no";
    uint8_t charge_level = data->power->getChargeLevel();
    uint16_t charge_level_scaled = (int16_t)charge_level * 100 / 255;
    int32_t current = data->power->getCurrent();

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

static void onPowerEnabledChanged(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        auto* data = static_cast<Data*>(lv_event_get_user_data(event));
        if (data->power->isChargingEnabled() != is_on) {
            data->power->setChargingEnabled(is_on);
            updateUi(data);
        }
    }
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    auto* data = static_cast<Data*>(app.getData());

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
    lv_obj_add_event_cb(enable_switch, onPowerEnabledChanged, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_set_align(enable_switch, LV_ALIGN_RIGHT_MID);

    data->enable_switch = enable_switch;
    data->charge_state = lv_label_create(wrapper);
    data->charge_level = lv_label_create(wrapper);
    data->current = lv_label_create(wrapper);

    updateUi(data);
    data->update_timer->start(ms_to_ticks(1000));
}

static void onHide(TT_UNUSED AppContext& app) {
    auto* data = static_cast<Data*>(app.getData());
    data->update_timer->stop();
}

static void onStart(AppContext& app) {
    auto* data = new Data();
    data->update_timer = new Timer(Timer::TypePeriodic, &updateUi, data);
    data->power = getConfiguration()->hardware->power;
    assert(data->power != nullptr); // The Power app only shows up on supported devices
    app.setData(data);
}

static void onStop(AppContext& app) {
    auto* data = static_cast<Data*>(app.getData());
    delete data->update_timer;
    delete data;
}

extern const AppManifest manifest = {
    .id = "Power",
    .name = "Power",
    .icon = TT_ASSETS_APP_ICON_POWER_SETTINGS,
    .type = TypeSettings,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
    .onHide = onHide
};

} // namespace
