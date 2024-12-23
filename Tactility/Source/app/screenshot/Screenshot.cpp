#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include "ScreenshotUi.h"
#include <memory>

namespace tt::app::screenshot {

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto ui = std::static_pointer_cast<ScreenshotUi>(app.getData());
    create_ui(app, ui, parent);
}

static void onStart(AppContext& app) {
    auto ui = std::shared_ptr<ScreenshotUi>(new ScreenshotUi());
    app.setData(ui); // Ensure data gets deleted when no more in use
}

extern const AppManifest manifest = {
    .id = "Screenshot",
    .name = "_Screenshot", // So it gets put at the bottom of the desktop and becomes less visible on small screen devices
    .icon = LV_SYMBOL_IMAGE,
    .type = TypeSystem,
    .onStart = onStart,
    .onShow = onShow,
};

} // namespace

#endif
