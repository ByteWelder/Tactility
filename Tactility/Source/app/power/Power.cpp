#include "app/AppContext.h"
#include "Assets.h"
#include "lvgl.h"
#include "Tactility.h"
#include "Timer.h"
#include "lvgl/LvglSync.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"

namespace tt::app::power {

#define TAG "power"

extern const AppManifest manifest;
static void onTimer(TT_UNUSED std::shared_ptr<void> context);

struct Data {
    Timer update_timer = Timer(Timer::TypePeriodic, &onTimer, nullptr);
    std::shared_ptr<tt::hal::Power> power = getConfiguration()->hardware->power();
    lv_obj_t* enable_switch = nullptr;
    lv_obj_t* battery_voltage = nullptr;
    lv_obj_t* charge_state = nullptr;
    lv_obj_t* charge_level = nullptr;
    lv_obj_t* current = nullptr;
};

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<Data> _Nullable optData() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return std::static_pointer_cast<Data>(app->getData());
    } else {
        return nullptr;
    }
}

static void updateUi(std::shared_ptr<Data> data) {
    const char* charge_state;
    hal::Power::MetricData metric_data;
    if (data->power->getMetric(hal::Power::MetricType::IS_CHARGING, metric_data)) {
        charge_state = metric_data.valueAsBool ? "yes" : "no";
    } else {
        charge_state = "N/A";
    }

    uint8_t charge_level;
    bool charge_level_scaled_set = false;
    if (data->power->getMetric(hal::Power::MetricType::CHARGE_LEVEL, metric_data)) {
        charge_level = metric_data.valueAsUint8;
        charge_level_scaled_set = true;
    }

    bool charging_enabled_set = data->power->supportsChargeControl();
    bool charging_enabled = data->power->supportsChargeControl() && data->power->isAllowedToCharge();

    int32_t current;
    bool current_set = false;
    if (data->power->getMetric(hal::Power::MetricType::CURRENT, metric_data)) {
        current = metric_data.valueAsInt32;
        current_set = true;
    }

    uint32_t battery_voltage;
    bool battery_voltage_set = false;
    if (data->power->getMetric(hal::Power::MetricType::BATTERY_VOLTAGE, metric_data)) {
        battery_voltage = metric_data.valueAsUint32;
        battery_voltage_set = true;
    }

    lvgl::lock(kernel::millisToTicks(1000));

    if (charging_enabled_set) {
        lv_obj_set_state(data->enable_switch, LV_STATE_CHECKED, charging_enabled);
        lv_obj_remove_flag(data->enable_switch, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(data->enable_switch, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text_fmt(data->charge_state, "Charging: %s", charge_state);

    if (battery_voltage_set) {
        lv_label_set_text_fmt(data->battery_voltage, "Battery voltage: %lu mV", battery_voltage);
    } else {
        lv_label_set_text_fmt(data->battery_voltage, "Battery voltage: N/A");
    }

    if (charge_level_scaled_set) {
        lv_label_set_text_fmt(data->charge_level, "Charge level: %d%%", charge_level);
    } else {
        lv_label_set_text_fmt(data->charge_level, "Charge level: N/A");
    }

    if (current_set) {
        lv_label_set_text_fmt(data->current, "Current: %ld mAh", current);
    } else {
        lv_label_set_text_fmt(data->current, "Current: N/A");
    }

    lvgl::unlock();
}

static void onTimer(TT_UNUSED std::shared_ptr<void> context) {
    auto data = optData();
    if (data != nullptr) {
        updateUi(data);
    }
}

static void onPowerEnabledChanged(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);

        auto data = optData();
        if (data != nullptr) {
            if (data->power->isAllowedToCharge() != is_on) {
                data->power->setAllowedToCharge(is_on);
                updateUi(data);
            }
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

    auto data = std::static_pointer_cast<Data>(app.getData());

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
    lv_obj_add_event_cb(enable_switch, onPowerEnabledChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_align(enable_switch, LV_ALIGN_RIGHT_MID);

    data->enable_switch = enable_switch;
    data->charge_state = lv_label_create(wrapper);
    data->charge_level = lv_label_create(wrapper);
    data->battery_voltage = lv_label_create(wrapper);
    data->current = lv_label_create(wrapper);

    updateUi(data);
    data->update_timer.start(kernel::millisToTicks(1000));
}

static void onHide(TT_UNUSED AppContext& app) {
    auto data = std::static_pointer_cast<Data>(app.getData());
    data->update_timer.stop();
}

static void onStart(AppContext& app) {
    auto data = std::make_shared<Data>();
    app.setData(data);
    assert(data->power != nullptr); // The Power app only shows up on supported devices
}

extern const AppManifest manifest = {
    .id = "Power",
    .name = "Power",
    .icon = TT_ASSETS_APP_ICON_POWER_SETTINGS,
    .type = TypeSettings,
    .onStart = onStart,
    .onShow = onShow,
    .onHide = onHide
};

} // namespace
