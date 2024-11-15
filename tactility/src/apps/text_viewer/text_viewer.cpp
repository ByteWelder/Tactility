#include "text_viewer.h"
#include "log.h"
#include "lvgl.h"
#include "ui/label_utils.h"
#include "ui/style.h"
#include "ui/toolbar.h"

#define TAG "text_viewer"

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_no_padding(wrapper);
    tt_lv_obj_set_style_bg_invisible(wrapper);

    lv_obj_t* text = lv_label_create(wrapper);
    lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
    Bundle bundle = tt_app_get_parameters(app);
    if (tt_bundle_has_string(bundle, TEXT_VIEWER_FILE_ARGUMENT)) {
        const char* filepath = tt_bundle_get_string(bundle, TEXT_VIEWER_FILE_ARGUMENT);
        TT_LOG_I(TAG, "Opening %s", filepath);
        tt_lv_label_set_text_file(text, filepath);
    }
}

extern const AppManifest text_viewer_app = {
    .id = "text_viewer",
    .name = "Text Viewer",
    .icon = NULL,
    .type = AppTypeHidden,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .on_hide = NULL
};
