#include "AlertDialog.h"
#include "lvgl.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"
#include <StringUtils.h>
#include <TactilityCore.h>

namespace tt::app::alertdialog {

#define PARAMETER_BUNDLE_KEY_TITLE "title"
#define PARAMETER_BUNDLE_KEY_MESSAGE "message"
#define PARAMETER_BUNDLE_KEY_BUTTON_LABELS "buttonLabels"
#define RESULT_BUNDLE_KEY_INDEX "index"

#define PARAMETER_ITEM_CONCATENATION_TOKEN ";;"
#define DEFAULT_TITLE "Select..."

#define TAG "selection_dialog"

extern const AppManifest manifest;

void start(const std::string& title, const std::string& message, const std::vector<std::string>& buttonLabels) {
    std::string items_joined = string::join(buttonLabels, PARAMETER_ITEM_CONCATENATION_TOKEN);
    auto bundle = std::make_shared<Bundle>();
    bundle->putString(PARAMETER_BUNDLE_KEY_TITLE, title);
    bundle->putString(PARAMETER_BUNDLE_KEY_MESSAGE, message);
    bundle->putString(PARAMETER_BUNDLE_KEY_BUTTON_LABELS, items_joined);
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


class AlertDialogApp : public App {

private:

    static void onButtonClickedCallback(lv_event_t* e) {
        auto appContext = service::loader::getCurrentAppContext();
        assert(appContext != nullptr);
        auto app = std::static_pointer_cast<AlertDialogApp>(appContext->getApp());
        app->onButtonClicked(e);
    }

    void onButtonClicked(lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        auto index = reinterpret_cast<std::size_t>(lv_event_get_user_data(e));
        TT_LOG_I(TAG, "Selected item at index %d", index);

        auto bundle = std::make_unique<Bundle>();
        bundle->putInt32(RESULT_BUNDLE_KEY_INDEX, (int32_t)index);
        setResult(app::Result::Ok, std::move(bundle));

        service::loader::stopApp();
    }

    static void createButton(lv_obj_t* parent, const std::string& text, size_t index) {
        lv_obj_t* button = lv_button_create(parent);
        lv_obj_t* button_label = lv_label_create(button);
        lv_obj_align(button_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(button_label, text.c_str());
        lv_obj_add_event_cb(button, onButtonClickedCallback, LV_EVENT_SHORT_CLICKED, (void*)index);
    }
public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto parameters = app.getParameters();
        tt_check(parameters != nullptr, "Parameters missing");

        std::string title = getTitleParameter(app.getParameters());
        lv_obj_t* toolbar = lvgl::toolbar_create(parent, title);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* message_label = lv_label_create(parent);
        lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_width(message_label, LV_PCT(80));

        std::string message;
        if (parameters->optString(PARAMETER_BUNDLE_KEY_MESSAGE, message)) {
            lv_label_set_text(message_label, message.c_str());
            lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
        }

        lv_obj_t* button_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(button_wrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(button_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(button_wrapper, 0, 0);
        lv_obj_set_flex_align(button_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(button_wrapper, 0, 0);
        lv_obj_align(button_wrapper, LV_ALIGN_BOTTOM_MID, 0, -4);

        std::string items_concatenated;
        if (parameters->optString(PARAMETER_BUNDLE_KEY_BUTTON_LABELS, items_concatenated)) {
            std::vector<std::string> labels = string::split(items_concatenated, PARAMETER_ITEM_CONCATENATION_TOKEN);
            if (labels.empty() || labels.front().empty()) {
                TT_LOG_E(TAG, "No items provided");
                setResult(Result::Error);
                service::loader::stopApp();
            } else if (labels.size() == 1) {
                auto result_bundle = std::make_unique<Bundle>();
                result_bundle->putInt32(RESULT_BUNDLE_KEY_INDEX, 0);
                setResult(Result::Ok, std::move(result_bundle));
                service::loader::stopApp();
                TT_LOG_W(TAG, "Auto-selecting single item");
            } else {
                size_t index = 0;
                for (const auto& label: labels) {
                    createButton(button_wrapper, label, index++);
                }
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "AlertDialog",
    .name = "Alert Dialog",
    .type = Type::Hidden,
    .createApp = create<AlertDialogApp>
};

}
