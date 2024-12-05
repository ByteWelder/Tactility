#include "ScreenshotUi.h"

namespace tt::app::screenshot {

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto* ui = static_cast<ScreenshotUi*>(app.getData());
    create_ui(app, ui, parent);
}

static void onStart(AppContext& app) {
    auto* ui = static_cast<ScreenshotUi*>(malloc(sizeof(ScreenshotUi)));
    app.setData(ui);
}

static void onStop(AppContext& app) {
    auto* ui = static_cast<ScreenshotUi*>(app.getData());
    free(ui);
}

extern const AppManifest manifest = {
    .id = "Screenshot",
    .name = "_Screenshot", // So it gets put at the bottom of the desktop and becomes less visible on small screen devices
    .icon = LV_SYMBOL_IMAGE,
    .type = TypeSystem,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
};

} // namespace
