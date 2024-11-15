#include "lvgl_keypad.h"

static lv_indev_t* keyboard_device = NULL;

bool tt_lvgl_keypad_is_available() {
    return keyboard_device != NULL;
}

void tt_lvgl_keypad_set_indev(lv_indev_t* device) {
    keyboard_device = device;
}

void tt_lvgl_keypad_activate(lv_group_t* group) {
    if (keyboard_device != NULL) {
        lv_indev_set_group(keyboard_device, group);
    }
}

void tt_lvgl_keypad_deactivate() {
    if (keyboard_device != NULL) {
        lv_indev_set_group(keyboard_device, NULL);
    }
}
