#include "ScreenshotUi.h"

namespace tt::app::screenshot {

static void on_show(App app, lv_obj_t* parent) {
    auto* ui = static_cast<ScreenshotUi*>(tt_app_get_data(app));
    create_ui(app, ui, parent);
}

static void on_start(App app) {
    auto* ui = static_cast<ScreenshotUi*>(malloc(sizeof(ScreenshotUi)));
    tt_app_set_data(app, ui);
}

static void on_stop(App app) {
    auto* ui = static_cast<ScreenshotUi*>(tt_app_get_data(app));
    free(ui);
}

extern const Manifest manifest = {
    .id = "Screenshot",
    .name = "_Screenshot", // So it gets put at the bottom of the desktop and becomes less visible on small screen devices
    .icon = LV_SYMBOL_IMAGE,
    .type = TypeSystem,
    .on_start = &on_start,
    .on_stop = &on_stop,
    .on_show = &on_show,
};

} // namespace
