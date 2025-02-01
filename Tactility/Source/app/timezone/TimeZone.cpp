#include "app/timezone/TimeZone.h"
#include "app/AppManifest.h"
#include "app/AppContext.h"
#include "service/loader/Loader.h"
#include "lvgl.h"
#include "lvgl/Toolbar.h"
#include "Partitions.h"
#include "lvgl/LvglSync.h"
#include "service/gui/Gui.h"
#include <memory>
#include <StringUtils.h>
#include <Timer.h>

namespace tt::app::timezone {

#define TAG "timezone_select"

#define RESULT_BUNDLE_CODE_INDEX "code"
#define RESULT_BUNDLE_NAME_INDEX "name"

extern const AppManifest manifest;

struct TimeZoneEntry {
    std::string name;
    std::string code;
};

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

// region Result

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

void setResultName(Bundle& bundle, const std::string& name) {
    bundle.putString(RESULT_BUNDLE_NAME_INDEX, name);
}

void setResultCode(Bundle& bundle, const std::string& code) {
    bundle.putString(RESULT_BUNDLE_CODE_INDEX, code);
}

// endregion


class TimeZoneApp : public App {

private:

    Mutex mutex;
    std::vector<TimeZoneEntry> entries;
    std::unique_ptr<Timer> updateTimer;
    lv_obj_t* listWidget = nullptr;
    lv_obj_t* filterTextareaWidget = nullptr;

    static void onTextareaValueChangedCallback(TT_UNUSED lv_event_t* e) {
        auto* app = (TimeZoneApp*)lv_event_get_user_data(e);
        app->onTextareaValueChanged(e);
    }

    void onTextareaValueChanged(TT_UNUSED lv_event_t* e) {
        if (mutex.lock(100 / portTICK_PERIOD_MS)) {
            if (updateTimer->isRunning()) {
                updateTimer->stop();
            }

            updateTimer->start(500 / portTICK_PERIOD_MS);

            mutex.unlock();
        }
    }

    static void onListItemSelectedCallback(lv_event_t* e) {
        auto index = reinterpret_cast<std::size_t>(lv_event_get_user_data(e));
        auto appContext = service::loader::getCurrentAppContext();
        if (appContext != nullptr && appContext->getManifest().id == manifest.id) {
            auto app = std::static_pointer_cast<TimeZoneApp>(appContext->getApp());
            app->onListItemSelected(index);
        }
    }

    void onListItemSelected(std::size_t index) {
        TT_LOG_I(TAG, "Selected item at index %zu", index);

        auto& entry = entries[index];

        auto bundle = std::make_unique<Bundle>();
        setResultName(*bundle, entry.name);
        setResultCode(*bundle, entry.code);

        setResult(app::Result::Ok, std::move(bundle));

        service::loader::stopApp();
    }

    static void createListItem(lv_obj_t* list, const std::string& title, size_t index) {
        lv_obj_t* btn = lv_list_add_button(list, nullptr, title.c_str());
        lv_obj_add_event_cb(btn, &onListItemSelectedCallback, LV_EVENT_SHORT_CLICKED, (void*)index);
    }

    static void updateTimerCallback(std::shared_ptr<void> context) {
        auto appContext = service::loader::getCurrentAppContext();
        if (appContext != nullptr && appContext->getManifest().id == manifest.id) {
            auto app = std::static_pointer_cast<TimeZoneApp>(appContext->getApp());
            app->updateList();
        }
    }

    void readTimeZones(std::string filter) {
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
        std::vector<TimeZoneEntry> new_entries;
        while (fgets(line, 96, file)) {
            if (parseEntry(line, name, code)) {
                if (tt::string::lowercase(name).find(filter) != std::string::npos) {
                    count++;
                    new_entries.push_back({.name = name, .code = code});

                    // Safety guard
                    if (count > 50) {
                        // TODO: Show warning that we're not displaying a complete list
                        break;
                    }
                }
            } else {
                TT_LOG_E(TAG, "Parse error at line %lu", count);
            }
        }

        fclose(file);

        if (mutex.lock(100 / portTICK_PERIOD_MS)) {
            entries = std::move(new_entries);
            mutex.unlock();
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }

        TT_LOG_I(TAG, "Processed %lu entries", count);
    }

    void updateList() {
        if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
            std::string filter = tt::string::lowercase(std::string(lv_textarea_get_text(filterTextareaWidget)));
            readTimeZones(filter);
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
            return;
        }

        if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
            if (mutex.lock(100 / portTICK_PERIOD_MS)) {
                lv_obj_clean(listWidget);

                uint32_t index = 0;
                for (auto& entry : entries) {
                    createListItem(listWidget, entry.name, index);
                    index++;
                }

                mutex.unlock();
            }

            lvgl::unlock();
        }
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lvgl::toolbar_create(parent, app);

        auto* search_wrapper = lv_obj_create(parent);
        lv_obj_set_size(search_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(search_wrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(search_wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_all(search_wrapper, 0, 0);
        lv_obj_set_style_border_width(search_wrapper, 0, 0);

        auto* icon = lv_image_create(search_wrapper);
        lv_obj_set_style_margin_left(icon, 8, 0);
        lv_obj_set_style_image_recolor_opa(icon, 255, 0);
        lv_obj_set_style_image_recolor(icon, lv_theme_get_color_primary(parent), 0);

        std::string icon_path = app.getPaths()->getSystemPathLvgl("search.png");
        lv_image_set_src(icon, icon_path.c_str());
        lv_obj_set_style_image_recolor(icon, lv_theme_get_color_primary(parent), 0);

        auto* textarea = lv_textarea_create(search_wrapper);
        lv_textarea_set_placeholder_text(textarea, "e.g. Europe/Amsterdam");
        lv_textarea_set_one_line(textarea, true);
        lv_obj_add_event_cb(textarea, onTextareaValueChangedCallback, LV_EVENT_VALUE_CHANGED, this);
        filterTextareaWidget = textarea;
        lv_obj_set_flex_grow(textarea, 1);
        service::gui::keyboardAddTextArea(textarea);

        auto* list = lv_list_create(parent);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_set_flex_grow(list, 1);
        lv_obj_set_style_border_width(list, 0, 0);
        listWidget = list;
    }

    void onCreate(AppContext& app) override {
        updateTimer = std::make_unique<Timer>(Timer::Type::Once, updateTimerCallback, nullptr);
    }
};

extern const AppManifest manifest = {
    .id = "TimeZone",
    .name = "Select timezone",
    .type = Type::Hidden,
    .createApp = create<TimeZoneApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

}
