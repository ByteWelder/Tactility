#include "app/ManifestRegistry.h"
#include "Assets.h"
#include "Check.h"
#include "lvgl.h"
#include <algorithm>
#include "service/loader/Loader.h"

namespace tt::app::desktop {

static void onAppPressed(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const auto* manifest = static_cast<const AppManifest*>(lv_event_get_user_data(e));
        service::loader::startApp(manifest->id, false);
    }
}

static void createAppWidget(const AppManifest* manifest, void* parent) {
    tt_check(parent);
    auto* list = static_cast<lv_obj_t*>(parent);
    const void* icon = !manifest->icon.empty() ? manifest->icon.c_str() : TT_ASSETS_APP_ICON_FALLBACK;
    lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name.c_str());
    lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_CLICKED, (void*)manifest);
}

static void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) {
    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_center(list);

    auto manifests = getApps();
    std::sort(manifests.begin(), manifests.end(), SortAppManifestByName);

    lv_list_add_text(list, "User");
    for (const auto& manifest: manifests) {
        if (manifest->type == TypeUser) {
            createAppWidget(manifest, list);
        }
    }

    lv_list_add_text(list, "System");
    for (const auto& manifest: manifests) {
        if (manifest->type == TypeSystem) {
            createAppWidget(manifest, list);
        }
    }
}

extern const AppManifest manifest = {
    .id = "Desktop",
    .name = "Desktop",
    .type = TypeDesktop,
    .onShow = onShow,
};

} // namespace
