#include "Tactility/lvgl/Keyboard.h"

extern "C" {

void tt_lvgl_software_keyboard_show(lv_obj_t* textarea) {
    tt::lvgl::software_keyboard_show(textarea);
}

void tt_lvgl_software_keyboard_hide() {
    tt::lvgl::software_keyboard_hide();
}

bool tt_lvgl_software_keyboard_is_enabled() {
    return tt::lvgl::software_keyboard_is_enabled();
}

void tt_lvgl_software_keyboard_activate(lv_group_t* group) {
    tt::lvgl::software_keyboard_activate(group);
}

void tt_lvgl_software_keyboard_deactivate() {
    tt::lvgl::software_keyboard_deactivate();
}

bool tt_lvgl_hardware_keyboard_is_available() {
    return tt::lvgl::hardware_keyboard_is_available();
}

void tt_lvgl_hardware_keyboard_set_indev(lv_indev_t* device) {
    tt::lvgl::hardware_keyboard_set_indev(device);
}

}
