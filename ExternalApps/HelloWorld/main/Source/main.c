#include <tt_app_manifest.h>
#include <tt_lvgl_toolbar.h>

/**
 * Note: LVGL and Tactility methods need to be exposed manually from TactilityC/Source/tt_init.cpp
 * Only C is supported for now (C++ symbols fail to link)
 */
static void onShow(AppHandle app, void* data, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt_lvgl_toolbar_create_for_app(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

ExternalAppManifest manifest = {
    .onShow = onShow
};

int main(int argc, char* argv[]) {
    tt_app_register(&manifest);
    return 0;
}
