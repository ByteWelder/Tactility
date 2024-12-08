#include <stddef.h>
#include "TactilityC/app/App.h"
#include "TactilityC/lvgl/Toolbar.h"

/**
 * Note: LVGL and Tactility methods need to be exposed manually from TactilityC/Source/TactilityC.cpp
 * Only C is supported for now (C++ symbols fail to link)
 */
static void onShow(AppContextHandle context, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt_lvgl_toolbar_create(parent, context);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

int main(int argc, char* argv[]) {
    tt_set_app_manifest(
        "Hello World",
        NULL,
        NULL,
        NULL,
        onShow,
        NULL,
        NULL
    );
    return 0;
}
