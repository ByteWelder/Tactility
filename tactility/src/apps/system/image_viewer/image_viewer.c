#include "image_viewer.h"
#include "log.h"
#include "lvgl.h"
#include "ui/style.h"
#include "ui/toolbar.h"
#include <tactility_core.h>

#define TAG "image_viewer"

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_no_padding(wrapper);
    tt_lv_obj_set_style_bg_invisible(wrapper);

    lv_obj_t* image = lv_img_create(wrapper);
    lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);
    Bundle bundle = tt_app_get_parameters(app);
    if (tt_bundle_has_string(bundle, IMAGE_VIEWER_FILE_ARGUMENT)) {
        const char* file = tt_bundle_get_string(bundle, IMAGE_VIEWER_FILE_ARGUMENT);
        char* prefixed_path = malloc(strlen(file) + 3);
        tt_assert(prefixed_path != NULL);
        sprintf(prefixed_path, "A:%s", file);
        TT_LOG_I(TAG, "Opening %s", prefixed_path);
        lv_img_set_src(image, prefixed_path);
        free(prefixed_path);
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
