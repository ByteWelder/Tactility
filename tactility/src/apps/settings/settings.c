#include "app_manifest_registry.h"
#include "check.h"
#include "lvgl.h"
#include "services/loader/loader.h"
#include "ui/toolbar.h"

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
    lv_obj_t* btn = lv_list_add_btn(list, LV_SYMBOL_FILE, manifest->name);
    lv_obj_add_event_cb(btn, &on_app_pressed, LV_EVENT_CLICKED, (void*)manifest);
}

static void on_show(TT_UNUSED App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);

    tt_app_manifest_registry_for_each_of_type(AppTypeSettings, list, create_app_widget);
}

const AppManifest settings_app = {
    .id = "settings",
    .name = "Settings",
    .icon = LV_SYMBOL_SETTINGS,
    .type = AppTypeSystem,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &on_show,
    .on_hide = NULL
};
