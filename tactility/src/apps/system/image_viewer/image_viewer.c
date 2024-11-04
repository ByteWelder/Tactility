#include "image_viewer.h"
#include "log.h"
#include "lvgl.h"
#include "ui/toolbar.h"

#define TAG "image_viewer"

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt_toolbar_create_for_app(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* image = lv_img_create(parent);
    lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);
    Bundle bundle = tt_app_get_parameters(app);
    if (tt_bundle_has_string(bundle, IMAGE_VIEWER_FILE_ARGUMENT)) {
        const char* file = tt_bundle_get_string(bundle, IMAGE_VIEWER_FILE_ARGUMENT);
        TT_LOG_I(TAG, "Opening %s", file);
        lv_img_set_src(image, file);
    }
}

const AppManifest image_viewer_app = {
    .id = "image_viewer",
    .name = "Image Viewer",
    .icon = NULL,
    .type = AppTypeDesktop,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .on_hide = NULL
};
