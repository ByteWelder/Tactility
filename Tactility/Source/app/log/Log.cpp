#include <sstream>
#include <vector>
#include "lvgl.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"
#include "app/selectiondialog/SelectionDialog.h"
#include "service/loader/Loader.h"
#include "lvgl/LvglSync.h"

#define TAG "text_viewer"

namespace tt::app::log {

struct LogAppData {
    LogLevel filterLevel = LogLevel::Info;
    lv_obj_t* labelWidget = nullptr;
};

static bool shouldShowLog(LogLevel filterLevel, LogLevel logLevel) {
    if (filterLevel == LogLevel::None || logLevel == LogLevel::None) {
        return false;
    } else {
        return filterLevel >= logLevel;
    }
}

static void setLogEntries(lv_obj_t* label) {
    auto app = service::loader::getCurrentApp();
    if (app == nullptr) {
        return;
    }
    auto data = std::static_pointer_cast<LogAppData>(app->getData());
    auto filterLevel = data->filterLevel;

    unsigned int index;
    auto* entries = copyLogEntries(index);
    std::stringstream buffer;
    if (entries != nullptr) {
        for (unsigned int i = index; i < TT_LOG_ENTRY_COUNT; ++i) {
            if (shouldShowLog(filterLevel, entries[i].level)) {
                buffer << entries[i].message;
            }
        }
        if (index != 0) {
            for (unsigned int i = 0; i < index; ++i) {
                if (shouldShowLog(filterLevel, entries[i].level)) {
                    buffer << entries[i].message;
                }
            }
        }
        delete entries;
        if (!buffer.str().empty()) {
            lv_label_set_text(label, buffer.str().c_str());
        } else {
            lv_label_set_text(label, "No logs for the selected log level");
        }
    } else {
        lv_label_set_text(label, "Failed to load log");
    }
}

static void onLevelFilterPressed(TT_UNUSED lv_event_t* event) {
    std::vector<std::string> items = {
        "Verbose",
        "Debug",
        "Info",
        "Warning",
        "Error",
    };
    app::selectiondialog::start("Log Level", items);
}

static void updateViews() {
    auto app = service::loader::getCurrentApp();
    if (app == nullptr) {
        return;
    }
    auto data = std::static_pointer_cast<LogAppData>(app->getData());
    assert(data != nullptr);

    if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
        setLogEntries(data->labelWidget);
        lvgl::unlock();
    }
}

class LogApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto data = std::static_pointer_cast<LogAppData>(app.getData());

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_EDIT, onLevelFilterPressed, nullptr);

        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(wrapper, 1);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lvgl::obj_set_style_no_padding(wrapper);
        lvgl::obj_set_style_bg_invisible(wrapper);

        data->labelWidget = lv_label_create(wrapper);
        lv_obj_align(data->labelWidget, LV_ALIGN_CENTER, 0, 0);
        setLogEntries(data->labelWidget);
    }

    void onStart(AppContext& app) override {
        auto data = std::make_shared<LogAppData>();
        app.setData(data);
    }

    void onResult(AppContext& app, Result result, const Bundle& bundle) override {
        auto resultIndex = selectiondialog::getResultIndex(bundle);
        auto data = std::static_pointer_cast<LogAppData>(app.getData());
        if (result == Result::Ok) {
            switch (resultIndex) {
                case 0:
                    data->filterLevel = LogLevel::Verbose;
                    break;
                case 1:
                    data->filterLevel = LogLevel::Debug;
                    break;
                case 2:
                    data->filterLevel = LogLevel::Info;
                    break;
                case 3:
                    data->filterLevel = LogLevel::Warning;
                    break;
                case 4:
                    data->filterLevel = LogLevel::Error;
                    break;
                default:
                    break;
            }
        }

        updateViews();
    }
};

extern const AppManifest manifest = {
    .id = "Log",
    .name = "Log",
    .icon = LV_SYMBOL_LIST,
    .type = Type::System,
    .createApp = create<LogApp>
};

} // namespace
