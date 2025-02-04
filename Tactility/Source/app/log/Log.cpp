#include "Tactility/app/selectiondialog/SelectionDialog.h"
#include "Tactility/lvgl/Style.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/service/loader/Loader.h"

#include <sstream>
#include <vector>
#include <ranges>

#include <lvgl.h>

#define TAG "text_viewer"

namespace tt::app::log {

class LogApp : public App {

private:

    LogLevel filterLevel = LogLevel::Info;
    lv_obj_t* labelWidget = nullptr;

    static inline bool shouldShowLog(LogLevel filterLevel, LogLevel logLevel) {
        return (filterLevel != LogLevel::None) &&
            (logLevel != LogLevel::None) &&
            filterLevel >= logLevel;
    }

    void updateLogEntries() {
        std::size_t next_log_index;
        auto entries = copyLogEntries(next_log_index);
        std::stringstream buffer;

        if (next_log_index != 0) {
            long to_drop = TT_LOG_ENTRY_COUNT - next_log_index;
            for (auto entry : std::views::drop(*entries, (long)next_log_index)) {
                if (shouldShowLog(filterLevel, entry.level) && entry.message[0] != 0x00) {
                    buffer << entry.message;
                }
            }
        }

        for (auto entry : std::views::take(*entries, (long)next_log_index)) {
            if (shouldShowLog(filterLevel, entry.level) && entry.message[0] != 0x00) {
                buffer << entry.message;
            }
        }

        if (!buffer.str().empty()) {
            lv_label_set_text(labelWidget, buffer.str().c_str());
        } else {
            lv_label_set_text(labelWidget, "No logs for the selected log level");
        }
    }

    void updateViews() {
        if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
            updateLogEntries();
            lvgl::unlock();
        }
    }

    static void onLevelFilterPressedCallback(TT_UNUSED lv_event_t* event) {
        std::vector<std::string> items = {
            "Verbose",
            "Debug",
            "Info",
            "Warning",
            "Error",
        };
        app::selectiondialog::start("Log Level", items);
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lvgl::toolbar_add_button_action(toolbar, LV_SYMBOL_EDIT, onLevelFilterPressedCallback, this);

        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(wrapper, 1);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lvgl::obj_set_style_no_padding(wrapper);
        lvgl::obj_set_style_bg_invisible(wrapper);

        labelWidget = lv_label_create(wrapper);
        lv_obj_align(labelWidget, LV_ALIGN_CENTER, 0, 0);

        updateLogEntries();
    }

    void onResult(AppContext& app, Result result, std::unique_ptr<Bundle> bundle) override {
        if (result == Result::Ok && bundle != nullptr) {
            auto resultIndex = selectiondialog::getResultIndex(*bundle);
            switch (resultIndex) {
                case 0:
                    filterLevel = LogLevel::Verbose;
                    break;
                case 1:
                    filterLevel = LogLevel::Debug;
                    break;
                case 2:
                    filterLevel = LogLevel::Info;
                    break;
                case 3:
                    filterLevel = LogLevel::Warning;
                    break;
                case 4:
                    filterLevel = LogLevel::Error;
                    break;
                default:
                    break;
            }

            updateViews();
        }
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
