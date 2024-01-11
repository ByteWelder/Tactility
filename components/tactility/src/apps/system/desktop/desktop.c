#include "app_manifest_registry.h"
#include "check.h"
#include "lvgl.h"
#include "services/loader/loader.h"

static void on_open_app(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const AppManifest* manifest = lv_event_get_user_data(e);
        loader_start_app(manifest->id, false);
    }
}

static void add_app_to_list(const AppManifest* manifest, void* _Nullable parent) {
    furi_check(parent);
    lv_obj_t* list = (lv_obj_t*)parent;
    lv_obj_t* btn = lv_list_add_btn(list, LV_SYMBOL_FILE, manifest->name);
    lv_obj_add_event_cb(btn, &on_open_app, LV_EVENT_CLICKED, (void*)manifest);
}

static void desktop_show(Context* context, lv_obj_t* parent) {
    UNUSED(context);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_center(list);

    lv_list_add_text(list, "System");
    app_manifest_registry_for_each_of_type(AppTypeSystem, list, add_app_to_list);
    lv_list_add_text(list, "User");
    app_manifest_registry_for_each_of_type(AppTypeUser, list, add_app_to_list);
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
