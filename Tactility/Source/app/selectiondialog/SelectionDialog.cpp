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

void setItemsParameter(Bundle& bundle, const std::vector<std::string>& items) {
    std::string result = string_join(items, PARAMETER_ITEM_CONCATENATION_TOKEN);
    bundle.putString(PARAMETER_BUNDLE_KEY_ITEMS, result);
}

int32_t getResultIndex(const Bundle& bundle) {
    int32_t index = -1;
    bundle.optInt32(RESULT_BUNDLE_KEY_INDEX, index);
    return index;
}

void setResultIndex(std::shared_ptr<Bundle> bundle, int32_t index) {
    bundle->putInt32(RESULT_BUNDLE_KEY_INDEX, index);
}

void setTitleParameter(std::shared_ptr<Bundle> bundle, const std::string& title) {
    bundle->putString(PARAMETER_BUNDLE_KEY_TITLE, title);
}

static std::string getTitleParameter(std::shared_ptr<const Bundle> bundle) {
    std::string result;
    if (bundle->optString(PARAMETER_BUNDLE_KEY_TITLE, result)) {
        return result;
    } else {
        return DEFAULT_TITLE;
    }
}

static void onListItemSelected(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        size_t index = (size_t)(e->user_data);
        TT_LOG_I(TAG, "Selected item at index %d", index);
        tt::app::AppContext* app = service::loader::getCurrentApp();
        auto bundle = std::shared_ptr<Bundle>(new Bundle());
        setResultIndex(bundle, (int32_t)index);
        app->setResult(app::ResultOk, bundle);
        service::loader::stopApp();
    }
}

static void createChoiceItem(void* parent, const std::string& title, size_t index) {
    auto* list = static_cast<lv_obj_t*>(parent);
    lv_obj_t* btn = lv_list_add_button(list, nullptr, title.c_str());
    lv_obj_add_event_cb(btn, &onListItemSelected, LV_EVENT_CLICKED, (void*)index);
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    std::string title = getTitleParameter(app.getParameters());
    lvgl::toolbar_create(parent, title);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);

    auto parameters = app.getParameters();
    tt_check(parameters != nullptr, "No parameters");
    std::string items_concatenated;
    if (parameters->optString(PARAMETER_BUNDLE_KEY_ITEMS, items_concatenated)) {
        std::vector<std::string> items = string_split(items_concatenated, PARAMETER_ITEM_CONCATENATION_TOKEN);
        if (items.empty() || items.front().empty()) {
            TT_LOG_E(TAG, "No items provided");
            app.setResult(ResultError);
            service::loader::stopApp();
        } else if (items.size() == 1) {
            auto result_bundle = std::shared_ptr<Bundle>(new Bundle());
            setResultIndex(result_bundle, 0);
            app.setResult(ResultOk, result_bundle);
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
        app.setResult(ResultError);
        service::loader::stopApp();
    }
}

extern const AppManifest manifest = {
     .id = "SelectionDialog",
     .name = "Selection Dialog",
     .type = TypeHidden,
     .onShow = onShow
};

}
