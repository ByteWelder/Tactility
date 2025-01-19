#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include "app/App.h"
#include "app/AppManifest.h"
#include "app/screenshot/ScreenshotUi.h"
#include <memory>

namespace tt::app::screenshot {

class ScreenshotApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto ui = std::static_pointer_cast<ScreenshotUi>(app.getData());
        ui->createWidgets(app, parent);
    }

    void onStart(AppContext& app) override {
        auto ui = std::make_shared<ScreenshotUi>();
        app.setData(ui); // Ensure data gets deleted when no more in use
    }
};

extern const AppManifest manifest = {
    .id = "Screenshot",
    .name = "Screenshot",
    .icon = LV_SYMBOL_IMAGE,
    .type = Type::System,
    .createApp = create<ScreenshotApp>
};

} // namespace

#endif
