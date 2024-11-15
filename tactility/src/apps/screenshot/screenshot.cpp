#include "tactility_core.h"
#include "ui/toolbar.h"
#include "screenshot_ui.h"

static void on_show(App app, lv_obj_t* parent) {
    auto* ui = static_cast<ScreenshotUi*>(tt_app_get_data(app));
    create_screenshot_ui(app, ui, parent);
}

static void on_start(App app) {
    auto* ui = static_cast<ScreenshotUi*>(malloc(sizeof(ScreenshotUi)));
    tt_app_set_data(app, ui);
}

static void on_stop(App app) {
    auto* ui = static_cast<ScreenshotUi*>(tt_app_get_data(app));
    free(ui);
}

extern const AppManifest screenshot_app = {
    .id = "screenshot",
    .name = "Screenshot",
    .icon = LV_SYMBOL_IMAGE,
    .type = AppTypeSystem,
    .on_start = &on_start,
    .on_stop = &on_stop,
    .on_show = &on_show,
    .on_hide = nullptr
};
