#define LV_USE_PRIVATE_API 1 // For actual lv_obj_t declaration

#include "Tactility/lvgl/Toolbar.h"

#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/Style.h"
#include "Tactility/lvgl/Spinner.h"

namespace tt::lvgl {

typedef struct {
    lv_obj_t obj;
    lv_obj_t* title_label;
    lv_obj_t* close_button;
    lv_obj_t* close_button_image;
    lv_obj_t* action_container;
    uint8_t  action_count;
} Toolbar;

static void toolbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);

static const lv_obj_class_t toolbar_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = &toolbar_constructor,
    .destructor_cb = nullptr,
    .event_cb = nullptr,
    .user_data = nullptr,
    .name = nullptr,
    .width_def = LV_PCT(100),
    .height_def = TOOLBAR_HEIGHT,
    .editable = false,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(Toolbar),
    .theme_inheritable = false
};

static void stop_app(TT_UNUSED lv_event_t* event) {
    service::loader::stopApp();
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

    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_gap(obj, 0, 0);

    lv_obj_center(obj);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);

    toolbar->close_button = lv_button_create(obj);
    lv_obj_set_size(toolbar->close_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    lv_obj_set_style_pad_all(toolbar->close_button, 0, 0);
    lv_obj_set_style_pad_gap(toolbar->close_button, 0, 0);
    toolbar->close_button_image = lv_image_create(toolbar->close_button);
    lv_obj_align(toolbar->close_button_image, LV_ALIGN_CENTER, 0, 0);

    toolbar->title_label = lv_label_create(obj);
    lv_obj_set_style_text_font(toolbar->title_label, &lv_font_montserrat_18, 0); // TODO replace with size 18
    lv_label_set_text(toolbar->title_label, title.c_str());
    lv_obj_set_style_text_align(toolbar->title_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_flex_grow(toolbar->title_label, 1);
    int32_t title_offset_x = (TOOLBAR_HEIGHT - TOOLBAR_TITLE_FONT_HEIGHT - 8) / 4 * 3;
    // Margin top doesn't work
    lv_obj_set_style_pad_top(toolbar->title_label, title_offset_x, 0);
    lv_obj_set_style_margin_left(toolbar->title_label, 8, 0);
    // Hack for margin bug where buttons in flex get rendered more narrowly
    lv_obj_set_style_margin_right(toolbar->title_label, -8, 0);

    toolbar->action_container = lv_obj_create(obj);
    lv_obj_set_width(toolbar->action_container, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(toolbar->action_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(toolbar->action_container, 0, 0);
    lv_obj_set_style_border_width(toolbar->action_container, 0, 0);
    lv_obj_set_flex_align(toolbar->action_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    toolbar_set_nav_action(obj, LV_SYMBOL_CLOSE, &stop_app, nullptr);

    return obj;
}

lv_obj_t* toolbar_create(lv_obj_t* parent, const app::AppContext& app) {
    return toolbar_create(parent, app.getManifest().name);
}

void toolbar_set_title(lv_obj_t* obj, const std::string& title) {
    auto* toolbar = (Toolbar*)obj;
    lv_label_set_text(toolbar->title_label, title.c_str());
}

void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data) {
    auto* toolbar = (Toolbar*)obj;
    lv_obj_add_event_cb(toolbar->close_button, callback, LV_EVENT_SHORT_CLICKED, user_data);
    lv_image_set_src(toolbar->close_button_image, icon); // e.g. LV_SYMBOL_CLOSE
}

lv_obj_t* toolbar_add_button_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data) {
    auto* toolbar = (Toolbar*)obj;
    tt_check(toolbar->action_count < TOOLBAR_ACTION_LIMIT, "max actions reached");
    toolbar->action_count++;

    lv_obj_t* action_button = lv_button_create(toolbar->action_container);
    lv_obj_set_size(action_button, TOOLBAR_HEIGHT - 4, TOOLBAR_HEIGHT - 4);
    lv_obj_set_style_pad_all(action_button, 0, 0);
    lv_obj_set_style_pad_gap(action_button, 0, 0);
    lv_obj_add_event_cb(action_button, callback, LV_EVENT_SHORT_CLICKED, user_data);
    lv_obj_t* action_button_image = lv_image_create(action_button);
    lv_image_set_src(action_button_image, icon);
    lv_obj_align(action_button_image, LV_ALIGN_CENTER, 0, 0);

    return action_button;
}

lv_obj_t* toolbar_add_switch_action(lv_obj_t* obj) {
    auto* toolbar = (Toolbar*)obj;
    lv_obj_t* widget = lv_switch_create(toolbar->action_container);
    lv_obj_set_style_margin_top(widget, 4, 0); // Because aligning doesn't work
    lv_obj_set_style_margin_right(widget, 4, 0);
    return widget;
}

lv_obj_t* toolbar_add_spinner_action(lv_obj_t* obj) {
    auto* toolbar = (Toolbar*)obj;
    return tt::lvgl::spinner_create(toolbar->action_container);
}

} // namespace
