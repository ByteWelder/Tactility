#include "LvglKeypad.h"

namespace tt::lvgl {

static lv_indev_t* keyboard_device = NULL;

bool keypad_is_available() {
    return keyboard_device != NULL;
}

void keypad_set_indev(lv_indev_t* device) {
    keyboard_device = device;
}

void keypad_activate(lv_group_t* group) {
    if (keyboard_device != NULL) {
        lv_indev_set_group(keyboard_device, group);
    }
}

void keypad_deactivate() {
    if (keyboard_device != NULL) {
        lv_indev_set_group(keyboard_device, NULL);
    }
}

} // namespace
