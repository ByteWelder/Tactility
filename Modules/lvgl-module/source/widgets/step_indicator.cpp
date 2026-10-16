// SPDX-License-Identifier: Apache-2.0
#define LV_USE_PRIVATE_API 1 // For the lv_obj_class_t declaration

#include <lvgl/widgets/step_indicator.h>

static void step_indicator_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    lv_obj_set_clickable(obj, false);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

static void step_indicator_segment_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    lv_obj_set_clickable(obj, false);
    lv_obj_set_scrollable(obj, false);
    // The segments share the indicator's width
    lv_obj_set_flex_grow(obj, 1);
}

// Not static: the theme styles step indicators and their segments by these classes
extern "C" const lv_obj_class_t lvgl_step_indicator_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = step_indicator_constructor,
    .destructor_cb = nullptr,
    .event_cb = nullptr,
    .user_data = nullptr,
    .name = "step_indicator",
    .width_def = LV_PCT(100),
    .height_def = LV_SIZE_CONTENT,
    .editable = LV_OBJ_CLASS_EDITABLE_INHERIT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
    .instance_size = 0,
    .theme_inheritable = LV_OBJ_CLASS_THEME_INHERITABLE_FALSE
};

extern "C" const lv_obj_class_t lvgl_step_indicator_segment_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = step_indicator_segment_constructor,
    .destructor_cb = nullptr,
    .event_cb = nullptr,
    .user_data = nullptr,
    .name = "step_indicator_segment",
    .width_def = 0,
    .height_def = 0,
    .editable = LV_OBJ_CLASS_EDITABLE_INHERIT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
    .instance_size = 0,
    .theme_inheritable = LV_OBJ_CLASS_THEME_INHERITABLE_FALSE
};

extern "C" {

lv_obj_t* lvgl_step_indicator_create(lv_obj_t* parent) {
    lv_obj_t* obj = lv_obj_class_create_obj(&lvgl_step_indicator_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lvgl_step_indicator_set_step_count(lv_obj_t* obj, uint32_t count) {
    uint32_t current = lv_obj_get_child_count(obj);
    while (current > count) {
        lv_obj_delete(lv_obj_get_child(obj, -1));
        current--;
    }
    while (current < count) {
        lv_obj_t* segment = lv_obj_class_create_obj(&lvgl_step_indicator_segment_class, obj);
        lv_obj_class_init_obj(segment);
        current++;
    }
}

void lvgl_step_indicator_set_finished_count(lv_obj_t* obj, uint32_t finished_count) {
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; i++) {
        // The theme shows finished steps by the checked state
        lv_obj_set_state(lv_obj_get_child(obj, static_cast<int32_t>(i)), LV_STATE_CHECKED, i < finished_count);
    }
}

}
