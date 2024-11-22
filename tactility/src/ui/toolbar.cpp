#define LV_USE_PRIVATE_API 1 // For actual lv_obj_t declaration
#include "toolbar.h"

#include "services/loader/loader_.h"
#include "ui/spacer.h"
#include "ui/style.h"

namespace tt::lvgl {

typedef struct {
    lv_obj_t obj;
    lv_obj_t* title_label;
    lv_obj_t* close_button;
    lv_obj_t* close_button_image;
    lv_obj_t* action_container;
    ToolbarAction* action_array[TOOLBAR_ACTION_LIMIT];
    uint8_t  action_count;
} Toolbar;

static void toolbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);

static const lv_obj_class_t toolbar_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = &toolbar_constructor,
    .destructor_cb = nullptr,
    .width_def = LV_PCT(100),
    .height_def = TOOLBAR_HEIGHT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(Toolbar),
};

static void stop_app(TT_UNUSED lv_event_t* event) {
    service::loader::stop_app();
}

static void toolbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    LV_TRACE_OBJ_CREATE("finished");
}

lv_obj_t* toolbar_create(lv_obj_t* parent, const std::string& title) {
    LV_LOG_INFO("begin");
    lv_obj_t* obj = lv_obj_class_create_obj(&toolbar_class, parent);
    lv_obj_class_init_obj(obj);

    auto* toolbar = (Toolbar*)obj;

    obj_set_style_no_padding(obj);
    lv_obj_center(obj);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);

    int32_t title_offset_x = (TOOLBAR_HEIGHT - TOOLBAR_TITLE_FONT_HEIGHT - 8) / 4 * 3;
    int32_t title_offset_y = (TOOLBAR_HEIGHT - TOOLBAR_TITLE_FONT_HEIGHT - 8) / 2;

    toolbar->close_button = lv_button_create(obj);
    lv_obj_set_size(toolbar->close_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    obj_set_style_no_padding(toolbar->close_button);
    toolbar->close_button_image = lv_image_create(toolbar->close_button);
    lv_obj_align(toolbar->close_button_image, LV_ALIGN_CENTER, 0, 0);

    // Need spacer to avoid button press glitch animation
    spacer_create(obj, title_offset_x, 1);

    lv_obj_t* label_container = lv_obj_create(obj);
    obj_set_style_no_padding(label_container);
    lv_obj_set_style_border_width(label_container, 0, 0);
    lv_obj_set_height(label_container, LV_PCT(100)); // 2% less due to 4px translate (it's not great, but it works)
    lv_obj_set_flex_grow(label_container, 1);

    toolbar->title_label = lv_label_create(label_container);
    lv_obj_set_style_text_font(toolbar->title_label, &lv_font_montserrat_18, 0); // TODO replace with size 18
    lv_obj_set_height(toolbar->title_label, TOOLBAR_TITLE_FONT_HEIGHT);
    lv_label_set_text(toolbar->title_label, title.c_str());
    lv_obj_set_pos(toolbar->title_label, 0, title_offset_y);
    lv_obj_set_style_text_align(toolbar->title_label, LV_TEXT_ALIGN_LEFT, 0);

    toolbar->action_container = lv_obj_create(obj);
    lv_obj_set_width(toolbar->action_container, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(toolbar->action_container, 0, 0);
    lv_obj_set_style_border_width(toolbar->action_container, 0, 0);

    return obj;
}

lv_obj_t* toolbar_create_for_app(lv_obj_t* parent, App app) {
    const AppManifest& manifest = tt_app_get_manifest(app);
    lv_obj_t* toolbar = toolbar_create(parent, manifest.name);
    toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, &stop_app, nullptr);
    return toolbar;
}

void toolbar_set_title(lv_obj_t* obj, const std::string& title) {
    auto* toolbar = (Toolbar*)obj;
    lv_label_set_text(toolbar->title_label, title.c_str());
}

void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data) {
    auto* toolbar = (Toolbar*)obj;
    lv_obj_add_event_cb(toolbar->close_button, callback, LV_EVENT_CLICKED, user_data);
    lv_image_set_src(toolbar->close_button_image, icon); // e.g. LV_SYMBOL_CLOSE
}

uint8_t toolbar_add_action(lv_obj_t* obj, const char* icon, const char* text, lv_event_cb_t callback, void* user_data) {
    auto* toolbar = (Toolbar*)obj;
    uint8_t id = toolbar->action_count;
    tt_check(toolbar->action_count < TOOLBAR_ACTION_LIMIT, "max actions reached");
    toolbar->action_count++;

    lv_obj_t* action_button = lv_button_create(toolbar->action_container);
    lv_obj_set_size(action_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    obj_set_style_no_padding(action_button);
    lv_obj_add_event_cb(action_button, callback, LV_EVENT_CLICKED, user_data);
    lv_obj_t* action_button_image = lv_image_create(action_button);
    lv_image_set_src(action_button_image, icon);
    lv_obj_align(action_button_image, LV_ALIGN_CENTER, 0, 0);

    return id;
}

} // namespace
