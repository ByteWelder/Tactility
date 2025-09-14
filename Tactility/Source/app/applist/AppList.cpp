#include "Tactility/app/AppRegistration.h"
#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/Toolbar.h"

#include <Tactility/Assets.h>

#include <lvgl.h>
#include <algorithm>

namespace tt::app::applist {

class AppListApp : public App {

    static void onAppPressed(lv_event_t* e) {
        const auto* manifest = static_cast<const AppManifest*>(lv_event_get_user_data(e));
        service::loader::startApp(manifest->id);
    }

    static void createAppWidget(const std::shared_ptr<AppManifest>& manifest, lv_obj_t* list) {
        const void* icon = !manifest->icon.empty() ? manifest->icon.c_str() : TT_ASSETS_APP_ICON_FALLBACK;
        lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name.c_str());
        lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_SHORT_CLICKED, manifest.get());
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* list = lv_list_create(parent);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_align_to(list, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        auto toolbar_height = lv_obj_get_height(toolbar);
        auto parent_content_height = lv_obj_get_content_height(parent);
        lv_obj_set_height(list, parent_content_height - toolbar_height);

        auto manifests = getApps();
        std::ranges::sort(manifests, SortAppManifestByName);

        for (const auto& manifest: manifests) {
            if (manifest->type == Type::User || manifest->type == Type::System) {
                createAppWidget(manifest, list);
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "AppList",
    .name = "Apps",
    .type = Type::Hidden,
    .createApp = create<AppListApp>,
};

} // namespace
