#include "Tactility/lvgl/Keyboard.h"
#include "Tactility/service/gui/Gui.h"

namespace tt::lvgl {

static lv_indev_t* keyboard_device = nullptr;

void software_keyboard_show(lv_obj_t* textarea) {
    service::gui::softwareKeyboardShow(textarea);
}

void software_keyboard_hide() {
    service::gui::softwareKeyboardHide();
}

bool software_keyboard_is_enabled() {
    return service::gui::softwareKeyboardIsEnabled();
}

void software_keyboard_activate(lv_group_t* group) {
    if (keyboard_device != nullptr) {
        lv_indev_set_group(keyboard_device, group);
    }
}

void software_keyboard_deactivate() {
    if (keyboard_device != nullptr) {
        lv_indev_set_group(keyboard_device, nullptr);
    }
}

bool hardware_keyboard_is_available() {
    return keyboard_device != nullptr;
}

void hardware_keyboard_set_indev(lv_indev_t* device) {
    keyboard_device = device;
}

}
