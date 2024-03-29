#include "app_manifest_registry.h"
#include "assets.h"
#include "check.h"
#include "lvgl.h"
#include "services/loader/loader.h"

static void on_app_pressed(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const AppManifest* manifest = lv_event_get_user_data(e);
        loader_start_app(manifest->id, false, NULL);
    }
}

static void create_app_widget(const AppManifest* manifest, void* parent) {
    tt_check(parent);
    lv_obj_t* list = (lv_obj_t*)parent;
    const char* icon = manifest->icon ?: TT_ASSETS_APP_ICON_FALLBACK;
    lv_obj_t* btn = lv_list_add_button(list, icon, manifest->name);
    lv_obj_add_event_cb(btn, &on_app_pressed, LV_EVENT_CLICKED, (void*)manifest);
}

static void desktop_show(TT_UNUSED App app, lv_obj_t* parent) {
    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_center(list);

    lv_list_add_text(list, "User");
    tt_app_manifest_registry_for_each_of_type(AppTypeUser, list, create_app_widget);
    lv_list_add_text(list, "System");
    tt_app_manifest_registry_for_each_of_type(AppTypeSystem, list, create_app_widget);
}

const AppManifest desktop_app = {
    .id = "desktop",
    .name = "Desktop",
    .icon = NULL,
    .type = AppTypeDesktop,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &desktop_show,
    .on_hide = NULL
};
