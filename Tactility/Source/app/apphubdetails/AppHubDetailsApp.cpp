#include <Tactility/app/AppRegistration.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/lvgl/Toolbar.h>

#include <lvgl.h>

namespace tt::app::apphubdetails {

class AppHubDetailsApp final : public App {

    static void onInstallPressed(lv_event_t* e) {
    }

    static void onUninstallPressed(lv_event_t* e) {
    }

    static void onUpdatePressed(lv_event_t* e) {
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* toolbar = lvgl::toolbar_create(parent, app); // TODO: App name
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
        // TODO
    }
};

extern const AppManifest manifest = {
    .appId = "AppHubDetails",
    .appName = "App Details",
    .appCategory = Category::System,
    .createApp = create<AppHubDetailsApp>,
};

} // namespace
