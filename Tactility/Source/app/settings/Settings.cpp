#include "Tactility/app/ManifestRegistry.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/Assets.h>
#include <Tactility/Check.h>
#include <lvgl.h>

#include <algorithm>

namespace tt::app::settings {

static void onAppPressed(lv_event_t* e) {
    const auto* manifest = static_cast<const AppManifest*>(lv_event_get_user_data(e));
    service::loader::startApp(manifest->id);
}

static void createWidget(const std::shared_ptr<AppManifest>& manifest, void* parent) {
    tt_check(parent);
    auto* list = (lv_obj_t*)parent;
    const void* icon = !manifest->icon.empty() ? manifest->icon.c_str() : TT_ASSETS_APP_ICON_FALLBACK;
    lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name.c_str());
    lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_SHORT_CLICKED, (void*)manifest.get());
}

class SettingsApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lvgl::toolbar_create(parent, app);

        lv_obj_t* list = lv_list_create(parent);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_set_flex_grow(list, 1);

        auto manifests = getApps();
        std::sort(manifests.begin(), manifests.end(), SortAppManifestByName);
        for (const auto& manifest: manifests) {
            if (manifest->type == Type::Settings) {
                createWidget(manifest, list);
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "Settings",
    .name = "Settings",
    .icon = TT_ASSETS_APP_ICON_SETTINGS,
    .type = Type::Hidden,
    .createApp = create<SettingsApp>
};

} // namespace
