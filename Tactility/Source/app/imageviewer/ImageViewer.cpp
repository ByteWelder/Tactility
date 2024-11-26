#include "ImageViewer.h"
#include "Log.h"
#include "lvgl.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"

namespace tt::app::imageviewer {

#define TAG "image_viewer"

static void on_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lvgl::obj_set_style_no_padding(wrapper);
    lvgl::obj_set_style_bg_invisible(wrapper);

    lv_obj_t* image = lv_img_create(wrapper);
    lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);

    const Bundle& bundle = tt_app_get_parameters(app);
    std::string file_argument;
    if (bundle.optString(IMAGE_VIEWER_FILE_ARGUMENT, file_argument)) {
        std::string prefixed_path = "A:" + file_argument;
        TT_LOG_I(TAG, "Opening %s", prefixed_path.c_str());
        lv_img_set_src(image, prefixed_path.c_str());
    }
}

extern const Manifest manifest = {
    .id = "ImageViewer",
    .name = "Image Viewer",
    .type = TypeHidden,
    .on_show = &on_show
};

} // namespace
