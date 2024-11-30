#include "app/ManifestRegistry.h"
#include "Assets.h"
#include "Check.h"
#include "service/loader/Loader.h"
#include "lvgl/Toolbar.h"
#include "lvgl.h"
#include <algorithm>

namespace tt::app::settings {

static void on_app_pressed(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const auto* manifest = static_cast<const Manifest*>(lv_event_get_user_data(e));
        service::loader::start_app(manifest->id);
    }
}

static void create_app_widget(const Manifest* manifest, void* parent) {
    tt_check(parent);
    auto* list = (lv_obj_t*)parent;
    const void* icon = !manifest->icon.empty() ? manifest->icon.c_str() : TT_ASSETS_APP_ICON_FALLBACK;
    lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name.c_str());
    lv_obj_add_event_cb(btn, &on_app_pressed, LV_EVENT_CLICKED, (void*)manifest);
}

static void on_show(App& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);

    auto manifests = getApps();
    std::sort(manifests.begin(), manifests.end(), SortAppManifestByName);
    for (const auto& manifest: manifests) {
        if (manifest->type == TypeSettings) {
            create_app_widget(manifest, list);
        }
    }
}

extern const Manifest manifest = {
    .id = "Settings",
    .name = "Settings",
    .icon = TT_ASSETS_APP_ICON_SETTINGS,
    .type = TypeSystem,
    .onStart = nullptr,
    .onStop = nullptr,
    .onShow = &on_show,
    .onHide = nullptr
};

} // namespace
