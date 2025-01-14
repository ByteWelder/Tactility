#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include "app/screenshot/ScreenshotUi.h"
#include <memory>

namespace tt::app::screenshot {

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto ui = std::static_pointer_cast<ScreenshotUi>(app.getData());
    ui->createWidgets(app, parent);
}

static void onStart(AppContext& app) {
    auto ui = std::make_shared<ScreenshotUi>();
    app.setData(ui); // Ensure data gets deleted when no more in use
}

extern const AppManifest manifest = {
    .id = "Screenshot",
    .name = "Screenshot",
    .icon = LV_SYMBOL_IMAGE,
    .type = TypeSystem,
    .onStart = onStart,
    .onShow = onShow,
};

} // namespace

#endif
