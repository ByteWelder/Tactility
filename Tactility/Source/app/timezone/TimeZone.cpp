#include "TimeZone.h"
#include "app/AppManifest.h"
#include "app/AppContext.h"
#include "service/loader/Loader.h"
#include "lvgl.h"
#include "lvgl/Toolbar.h"
#include "Partitions.h"
#include "TactilityHeadless.h"
#include "lvgl/LvglSync.h"
#include <memory>
#include <StringUtils.h>

namespace tt::app::timezone {

#define TAG "timezone_select"

#define RESULT_BUNDLE_CODE_INDEX "code"
#define RESULT_BUNDLE_NAME_INDEX "name"

extern const AppManifest manifest;

struct TimeZoneEntry {
    std::string name;
    std::string code;
};

struct Data {
    Mutex mutex;
    std::vector<TimeZoneEntry> entries;
    lv_obj_t* list;
};

static void updateList(std::shared_ptr<Data>& data);

static bool parseEntry(const std::string& input, std::string& outName, std::string& outCode) {
    std::string partial_strip = input.substr(1, input.size() - 3);
    auto first_end_quote = partial_strip.find('"');
    if (first_end_quote == std::string::npos) {
        return false;
    } else {
        outName = partial_strip.substr(0, first_end_quote);
        outCode = partial_strip.substr(first_end_quote + 3);
        return true;
    }
}

std::string getResultName(const Bundle& bundle) {
    std::string result;
    bundle.optString(RESULT_BUNDLE_NAME_INDEX, result);
    return result;
}

std::string getResultCode(const Bundle& bundle) {
    std::string result;
    bundle.optString(RESULT_BUNDLE_CODE_INDEX, result);
    return result;
}

void setResultName(std::shared_ptr<Bundle>& bundle, const std::string& name) {
    bundle->putString(RESULT_BUNDLE_NAME_INDEX, name);
}

void setResultCode(std::shared_ptr<Bundle>& bundle, const std::string& code) {
    bundle->putString(RESULT_BUNDLE_CODE_INDEX, code);
}

static void onListItemSelected(lv_event_t* e) {
    auto index = reinterpret_cast<std::size_t>(lv_event_get_user_data(e));
    TT_LOG_I(TAG, "Selected item at index %zu", index);
    auto* app = service::loader::getCurrentApp();
    auto data = std::static_pointer_cast<Data>(app->getData());

    auto& entry = data->entries[index];

    auto bundle = std::make_shared<Bundle>();
    setResultName(bundle, entry.name);
    setResultCode(bundle, entry.code);
    app->setResult(app::ResultOk, bundle);

    service::loader::stopApp();
}

static void createListItem(lv_obj_t* list, const std::string& title, size_t index) {
    lv_obj_t* btn = lv_list_add_button(list, nullptr, title.c_str());
    lv_obj_add_event_cb(btn, &onListItemSelected, LV_EVENT_SHORT_CLICKED, (void*)index);
}

static void readTimeZones(const std::shared_ptr<Data>& data) {
    auto path = std::string(MOUNT_POINT_SYSTEM) + "/timezones.csv";
    auto* file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        TT_LOG_E(TAG, "Failed to open %s", path.c_str());
        return;
    }
    char line[96];
    std::string name;
    std::string code;
    uint32_t count = 0;
    std::vector<TimeZoneEntry> entries;
    while (fgets(line, 96, file)) {
        count++;
        if (parseEntry(line, name, code)) {
            entries.push_back({
                .name = name,
                .code = code
            });
        } else {
            TT_LOG_E(TAG, "Parse error at line %lu", count);
        }
    }

    if (data->mutex.lock(100 / portTICK_PERIOD_MS)) {
        data->entries = std::move(entries);
        data->mutex.unlock();
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
    }

    TT_LOG_I(TAG, "Processed %lu entries", count);
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto data = std::static_pointer_cast<Data>(app.getData());

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, app);

    auto* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    data->list = list;

}

static void updateList(std::shared_ptr<Data>& data) {
    if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
        uint32_t index = 0;
        for (auto& entry : data->entries) {
            createListItem(data->list, entry.name, index++);
        }
        lvgl::unlock();
    }
}

static void onReadTimeZone(std::shared_ptr<void> input) {
    auto data = std::static_pointer_cast<Data>(input);
    readTimeZones(data);
}

static void onStart(AppContext& app) {
    auto data = std::make_shared<Data>();
    app.setData(data);
    getMainDispatcher().dispatch(onReadTimeZone, data);
}

extern const AppManifest manifest = {
    .id = "TimeZone",
    .name = "Select timezone",
    .type = TypeHidden,
    .onStart = onStart,
    .onShow = onShow,
};

void start() {
    service::loader::startApp(manifest.id);
}

}
