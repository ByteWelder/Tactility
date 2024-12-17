#include <TactilityCore.h>
#include "ImageViewer.h"
#include "lvgl.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"
#include "StringUtils.h"

namespace tt::app::imageviewer {

extern const AppManifest manifest;

#define TAG "image_viewer"

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto wrapper = lv_obj_create(parent);
    lv_obj_set_size(wrapper, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lvgl::obj_set_style_no_padding(wrapper);

    auto toolbar = lvgl::toolbar_create(wrapper, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    auto* image_wrapper = lv_obj_create(wrapper);
    lv_obj_align_to(image_wrapper, toolbar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_width(image_wrapper, LV_PCT(100));
    auto parent_height = lv_obj_get_height(wrapper);
    lv_obj_set_height(image_wrapper, parent_height - TOOLBAR_HEIGHT);
    lv_obj_set_flex_flow(image_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(image_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lvgl::obj_set_style_no_padding(image_wrapper);
    lvgl::obj_set_style_bg_invisible(image_wrapper);

    auto* image = lv_image_create(image_wrapper);
    lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);

    auto* file_label = lv_label_create(wrapper);
    lv_obj_align_to(file_label, wrapper, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    std::shared_ptr<const Bundle> bundle = app.getParameters();
    tt_check(bundle != nullptr, "Parameters not set");
    std::string file_argument;
    if (bundle->optString(IMAGE_VIEWER_FILE_ARGUMENT, file_argument)) {
        std::string prefixed_path = "A:" + file_argument;
        TT_LOG_I(TAG, "Opening %s", prefixed_path.c_str());
        lv_img_set_src(image, prefixed_path.c_str());
        auto path = string::getLastPathSegment(file_argument);
        lv_label_set_text(file_label, path.c_str());
    } else {
        lv_label_set_text(file_label, "File not found");
    }
}

extern const AppManifest manifest = {
    .id = "ImageViewer",
    .name = "Image Viewer",
    .type = TypeHidden,
    .onShow = onShow
};

} // namespace
