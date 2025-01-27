#include "app/AppContext.h"
#include "app/ManifestRegistry.h"
#include "Check.h"
#include "lvgl.h"
#include "service/loader/Loader.h"

namespace tt::app::launcher {

static void onAppPressed(TT_UNUSED lv_event_t* e) {
    auto* appId = (const char*)lv_event_get_user_data(e);
    service::loader::startApp(appId);
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
    lv_obj_add_event_cb(apps_button, onAppPressed, LV_EVENT_SHORT_CLICKED, (void*)appId);
    lv_obj_set_style_image_recolor(button_image, lv_theme_get_color_primary(parent), 0);
    lv_obj_set_style_image_recolor_opa(button_image, LV_OPA_COVER, 0);
    // Ensure buttons are still tappable when asset fails to load
    lv_obj_set_size(button_image, 64, 64);

    auto* label = lv_label_create(wrapper);
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

    return wrapper;
}

class LauncherApp : public App {

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* wrapper = lv_obj_create(parent);

        lv_obj_align(wrapper, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_pad_all(wrapper, 0, 0);
        lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lv_obj_set_flex_grow(wrapper, 1);

        auto* display = lv_obj_get_display(parent);
        auto horizontal_px = lv_display_get_horizontal_resolution(display);
        auto vertical_px = lv_display_get_vertical_resolution(display);
        bool is_landscape_display = horizontal_px > vertical_px;
        if (is_landscape_display) {
            lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
        } else {
            lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        }

        int32_t available_width = lv_display_get_horizontal_resolution(display) - (3 * 80);
        int32_t padding = is_landscape_display ? std::min(available_width / 4, 64) : 0;

        auto paths = app.getPaths();
        auto apps_icon_path = paths->getSystemPathLvgl("icon_apps.png");
        auto files_icon_path = paths->getSystemPathLvgl("icon_files.png");
        auto settings_icon_path = paths->getSystemPathLvgl("icon_settings.png");
        createAppButton(wrapper, "Apps", apps_icon_path.c_str(), "AppList", 0);
        createAppButton(wrapper, "Files", files_icon_path.c_str(), "Files", padding);
        createAppButton(wrapper, "Settings", settings_icon_path.c_str(), "Settings", padding);
    }
};

extern const AppManifest manifest = {
    .id = "Launcher",
    .name = "Launcher",
    .type = Type::Launcher,
    .createApp = create<LauncherApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
