#include "ScreenshotUi.h"

namespace tt::app::screenshot {

static void on_show(App& app, lv_obj_t* parent) {
    auto* ui = static_cast<ScreenshotUi*>(app.getData());
    create_ui(app, ui, parent);
}

static void on_start(App& app) {
    auto* ui = static_cast<ScreenshotUi*>(malloc(sizeof(ScreenshotUi)));
    app.setData(ui);
}

static void on_stop(App& app) {
    auto* ui = static_cast<ScreenshotUi*>(app.getData());
    free(ui);
}

extern const Manifest manifest = {
    .id = "Screenshot",
    .name = "_Screenshot", // So it gets put at the bottom of the desktop and becomes less visible on small screen devices
    .icon = LV_SYMBOL_IMAGE,
    .type = TypeSystem,
    .onStart = &on_start,
    .onStop = &on_stop,
    .onShow = &on_show,
};

} // namespace
