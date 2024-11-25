#include "AppManifestRegistry.h"
#include "Assets.h"
#include "Check.h"
#include "service/loader/Loader.h"
#include "ui/Toolbar.h"
#include "lvgl.h"
#include <algorithm>

namespace tt::app::settings {

static void on_app_pressed(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const auto* manifest = static_cast<const AppManifest*>(lv_event_get_user_data(e));
        service::loader::start_app(manifest->id, false, Bundle());
    }
}

static void create_app_widget(const AppManifest* manifest, void* parent) {
    tt_check(parent);
    auto* list = (lv_obj_t*)parent;
    const void* icon = !manifest->icon.empty() ? manifest->icon.c_str() : TT_ASSETS_APP_ICON_FALLBACK;
    lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name.c_str());
    lv_obj_add_event_cb(btn, &on_app_pressed, LV_EVENT_CLICKED, (void*)manifest);
}

static void on_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);

    auto manifests = app_manifest_registry_get();
    std::sort(manifests.begin(), manifests.end(), SortAppManifestByName);
    for (const auto& manifest: manifests) {
        if (manifest->type == AppTypeSettings) {
            create_app_widget(manifest, list);
        }
    }
}

extern const AppManifest manifest = {
    .id = "Settings",
    .name = "Settings",
    .icon = TT_ASSETS_APP_ICON_SETTINGS,
    .type = AppTypeSystem,
    .on_start = nullptr,
    .on_stop = nullptr,
    .on_show = &on_show,
    .on_hide = nullptr
};

} // namespace
