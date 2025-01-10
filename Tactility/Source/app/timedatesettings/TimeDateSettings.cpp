#include <StringUtils.h>
#include "lvgl.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"
#include "app/timezone/TimeZone.h"
#include "Assets.h"
#include "Tactility.h"
#include "time/Time.h"
#include "lvgl/LvglSync.h"
#include <iomanip>

#define TAG "text_viewer"

namespace tt::app::timedatesettings {

extern const AppManifest manifest;

struct Data {
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    lv_obj_t* regionLabelWidget = nullptr;
};

std::string getPortNamesForDropdown() {
    std::vector<std::string> config_names;
    size_t port_index = 0;
    for (const auto& i2c_config: tt::getConfiguration()->hardware->i2c) {
        if (!i2c_config.name.empty()) {
            config_names.push_back(i2c_config.name);
        } else {
            std::stringstream stream;
            stream << "Port " << std::to_string(port_index);
            config_names.push_back(stream.str());
        }
        port_index++;
    }
    return tt::string::join(config_names, "\n");
}

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<Data> _Nullable optData() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return std::static_pointer_cast<Data>(app->getData());
    } else {
        return nullptr;
    }
}

static void onConfigureTimeZonePressed(TT_UNUSED lv_event_t* event) {
    timezone::start();
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto data = std::static_pointer_cast<Data>(app.getData());

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    auto* main_wrapper = lv_obj_create(parent);
    lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(main_wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(main_wrapper, 1);

    auto* wrapper = lv_obj_create(main_wrapper);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    auto* region_prefix_label = lv_label_create(wrapper);
    lv_label_set_text(region_prefix_label, "Region: ");
    lv_obj_align(region_prefix_label, LV_ALIGN_TOP_LEFT, 0, 8); // Shift to align with selection box

    auto* region_label = lv_label_create(wrapper);
    std::string timeZoneName = time::getTimeZoneName();
    if (timeZoneName.empty()) {
        timeZoneName = "not set";
    }
    data->regionLabelWidget = region_label;
    lv_label_set_text(region_label, timeZoneName.c_str());
    lv_obj_align_to(region_label, region_prefix_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    auto* region_button = lv_button_create(wrapper);
    lv_obj_align(region_button, LV_ALIGN_TOP_RIGHT, 0, 0);
    auto* region_button_image = lv_image_create(region_button);
    lv_obj_add_event_cb(region_button, onConfigureTimeZonePressed, LV_EVENT_SHORT_CLICKED, nullptr);
    lv_image_set_src(region_button_image, LV_SYMBOL_SETTINGS);
}

static void onStart(AppContext& app) {
    auto data = std::make_shared<Data>();
    app.setData(data);
}

static void onResult(AppContext& app, Result result, const Bundle& bundle) {
    if (result == ResultOk) {
        auto data = std::static_pointer_cast<Data>(app.getData());
        auto name = timezone::getResultName(bundle);
        auto code = timezone::getResultCode(bundle);
        TT_LOG_I(TAG, "Result name=%s code=%s", name.c_str(), code.c_str());
        time::setTimeZone(name, code);

        if (!name.empty()) {
            if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
                lv_label_set_text(data->regionLabelWidget, name.c_str());
                lvgl::unlock();
            }
        }
    }
}

extern const AppManifest manifest = {
    .id = "TimeDateSettings",
    .name = "Time & Date",
    .icon = TT_ASSETS_APP_ICON_TIME_DATE_SETTINGS,
    .type = TypeSettings,
    .onStart = onStart,
    .onShow = onShow,
    .onResult = onResult
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
