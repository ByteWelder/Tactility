#include "Tactility/lvgl/Keyboard.h"
#include "Tactility/service/gui/GuiService.h"

#include <Tactility/service/espnow/EspNowService.h>

namespace tt::lvgl {

static lv_indev_t* keyboard_device = nullptr;

void software_keyboard_show(lv_obj_t* textarea) {
    auto gui_service = service::gui::findService();
    if (gui_service != nullptr) {
        gui_service->softwareKeyboardShow(textarea);
    }
}

void software_keyboard_hide() {
    auto gui_service = service::gui::findService();
    if (gui_service != nullptr) {
        gui_service->softwareKeyboardHide();
    }
}

bool software_keyboard_is_enabled() {
    auto gui_service = service::gui::findService();
    if (gui_service != nullptr) {
        return gui_service->softwareKeyboardIsEnabled();
    } else {
        return false;
    }
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
