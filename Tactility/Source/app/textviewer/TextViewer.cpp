#include "Log.h"
#include "TextViewer.h"
#include "lvgl.h"
#include "lvgl/LabelUtils.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"

#define TAG "text_viewer"

namespace tt::app::textviewer {

static void onShow(AppContext& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lvgl::obj_set_style_no_padding(wrapper);
    lvgl::obj_set_style_bg_invisible(wrapper);

    lv_obj_t* label = lv_label_create(wrapper);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    const Bundle& bundle = app.getParameters();
    std::string file_argument;
    if (bundle.optString(TEXT_VIEWER_FILE_ARGUMENT, file_argument)) {
        TT_LOG_I(TAG, "Opening %s", file_argument.c_str());
        lvgl::label_set_text_file(label, file_argument.c_str());
    } else {
        lv_label_set_text_fmt(label, "Failed to load %s", file_argument.c_str());
    }
}

extern const AppManifest manifest = {
    .id = "TextViewer",
    .name = "Text Viewer",
    .type = TypeHidden,
    .onShow = onShow
};

} // namespace
