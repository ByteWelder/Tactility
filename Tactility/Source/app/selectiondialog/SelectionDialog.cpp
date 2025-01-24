#include "SelectionDialog.h"
#include "lvgl.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"
#include <StringUtils.h>
#include <TactilityCore.h>

namespace tt::app::selectiondialog {

#define PARAMETER_BUNDLE_KEY_TITLE "title"
#define PARAMETER_BUNDLE_KEY_ITEMS "items"
#define RESULT_BUNDLE_KEY_INDEX "index"

#define PARAMETER_ITEM_CONCATENATION_TOKEN ";;"
#define DEFAULT_TITLE "Select..."

#define TAG "selection_dialog"

extern const AppManifest manifest;

void start(const std::string& title, const std::vector<std::string>& items) {
    std::string items_joined = string::join(items, PARAMETER_ITEM_CONCATENATION_TOKEN);
    auto bundle = std::make_shared<Bundle>();
    bundle->putString(PARAMETER_BUNDLE_KEY_TITLE, title);
    bundle->putString(PARAMETER_BUNDLE_KEY_ITEMS, items_joined);
    service::loader::startApp(manifest.id, bundle);
}

int32_t getResultIndex(const Bundle& bundle) {
    int32_t index = -1;
    bundle.optInt32(RESULT_BUNDLE_KEY_INDEX, index);
    return index;
}

static std::string getTitleParameter(std::shared_ptr<const Bundle> bundle) {
    std::string result;
    if (bundle->optString(PARAMETER_BUNDLE_KEY_TITLE, result)) {
        return result;
    } else {
        return DEFAULT_TITLE;
    }
}

class SelectionDialogApp : public App {

private:

    static void onListItemSelectedCallback(lv_event_t* e) {
        auto appContext = service::loader::getCurrentAppContext();
        assert(appContext != nullptr);
        auto app = std::static_pointer_cast<SelectionDialogApp>(appContext->getApp());
        app->onListItemSelected(e);
    }

    void onListItemSelected(lv_event_t* e) {
        size_t index = reinterpret_cast<std::size_t>(lv_event_get_user_data(e));
        TT_LOG_I(TAG, "Selected item at index %d", index);
        auto bundle = std::make_unique<Bundle>();
        bundle->putInt32(RESULT_BUNDLE_KEY_INDEX, (int32_t)index);
        setResult(app::Result::Ok, std::move(bundle));
        service::loader::stopApp();
    }

    static void createChoiceItem(void* parent, const std::string& title, size_t index) {
        auto* list = static_cast<lv_obj_t*>(parent);
        lv_obj_t* btn = lv_list_add_button(list, nullptr, title.c_str());
        lv_obj_add_event_cb(btn, onListItemSelectedCallback, LV_EVENT_SHORT_CLICKED, (void*)index);
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        std::string title = getTitleParameter(app.getParameters());
        lvgl::toolbar_create(parent, title);

        lv_obj_t* list = lv_list_create(parent);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_set_flex_grow(list, 1);

        auto parameters = app.getParameters();
        tt_check(parameters != nullptr, "Parameters missing");
        std::string items_concatenated;
        if (parameters->optString(PARAMETER_BUNDLE_KEY_ITEMS, items_concatenated)) {
            std::vector<std::string> items = string::split(items_concatenated, PARAMETER_ITEM_CONCATENATION_TOKEN);
            if (items.empty() || items.front().empty()) {
                TT_LOG_E(TAG, "No items provided");
                setResult(Result::Error);
                service::loader::stopApp();
            } else if (items.size() == 1) {
                auto result_bundle = std::make_unique<Bundle>();
                result_bundle->putInt32(RESULT_BUNDLE_KEY_INDEX, 0);
                setResult(Result::Ok, std::move(result_bundle));
                service::loader::stopApp();
                TT_LOG_W(TAG, "Auto-selecting single item");
            } else {
                size_t index = 0;
                for (const auto& item: items) {
                    createChoiceItem(list, item, index++);
                }
            }
        } else {
            TT_LOG_E(TAG, "No items provided");
            setResult(Result::Error);
            service::loader::stopApp();
        }
    }
};

extern const AppManifest manifest = {
    .id = "SelectionDialog",
    .name = "Selection Dialog",
    .type = Type::Hidden,
    .createApp = create<SelectionDialogApp>
};

}
