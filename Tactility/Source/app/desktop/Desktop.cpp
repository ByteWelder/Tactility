#include "app/ManifestRegistry.h"
#include "Check.h"
#include "lvgl.h"
#include "service/loader/Loader.h"

namespace tt::app::desktop {

static void onAppPressed(TT_UNUSED lv_event_t* e) {
    auto* appId = (const char*)lv_event_get_user_data(e);
    service::loader::startApp(appId, false);
}

static lv_obj_t* createAppButton(lv_obj_t* parent, const char* title, const char* imageFile, const char* appId, int32_t buttonPaddingLeft) {
    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(wrapper, 0, 0);
    lv_obj_set_style_pad_left(wrapper, buttonPaddingLeft, 0);
    lv_obj_set_style_pad_right(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    auto* apps_button = lv_button_create(wrapper);
    lv_obj_set_style_pad_hor(apps_button, 0, 0);
    lv_obj_set_style_pad_top(apps_button, 0, 0);
    lv_obj_set_style_pad_bottom(apps_button, 16, 0);
    lv_obj_set_style_shadow_width(apps_button, 0, 0);
    lv_obj_set_style_border_width(apps_button, 0, 0);
    lv_obj_set_style_bg_color(apps_button, lv_color_white(), 0);

    auto* button_image = lv_image_create(apps_button);
    lv_image_set_src(button_image, imageFile);
    lv_obj_add_event_cb(apps_button, onAppPressed, LV_EVENT_CLICKED, (void*)appId);
    lv_obj_set_style_image_recolor(button_image, lv_theme_get_color_primary(parent), 0);
    lv_obj_set_style_image_recolor_opa(button_image, LV_OPA_COVER, 0);

    auto* label = lv_label_create(wrapper);
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

    return wrapper;
}

static void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) {
    auto* wrapper = lv_obj_create(parent);

    lv_obj_align(wrapper, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);

    auto* display = lv_obj_get_display(parent);
    auto orientation = lv_display_get_rotation(display);
    if (orientation == LV_DISPLAY_ROTATION_0 || orientation == LV_DISPLAY_ROTATION_180) {
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
    } else {
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    }

    int32_t available_width = lv_display_get_horizontal_resolution(display) - (3 * 80);
    int32_t padding = TT_MIN(available_width / 4, 64);

    createAppButton(wrapper, "Apps", "A:/assets/desktop_icon_apps.png", "AppList", 0);
    createAppButton(wrapper, "Files", "A:/assets/desktop_icon_files.png", "Files", padding);
    createAppButton(wrapper, "Settings", "A:/assets/desktop_icon_settings.png", "Settings", padding);
}

extern const AppManifest manifest = {
    .id = "Desktop",
    .name = "Desktop",
    .type = TypeDesktop,
    .onShow = onShow,
};

} // namespace
