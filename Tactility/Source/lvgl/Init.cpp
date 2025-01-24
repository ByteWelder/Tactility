#include "app/display/DisplaySettings.h"
#include "lvgl.h"
#include "hal/Configuration.h"
#include "hal/Display.h"
#include "hal/Touch.h"
#include "hal/Keyboard.h"
#include "lvgl/LvglKeypad.h"
#include "lvgl/LvglDisplay.h"
#include "kernel/SystemEvents.h"

namespace tt::lvgl {

#define TAG "lvglinit"

bool initDisplay(const hal::Configuration& config) {
    assert(config.createDisplay);
    auto* display = config.createDisplay();
    if (!display->start()) {
        TT_LOG_E(TAG, "Display start failed");
        return false;
    }

    lv_display_t* lvgl_display = display->getLvglDisplay();
    assert(lvgl_display);

    if (display->supportsBacklightDuty()) {
        display->setBacklightDuty(0);
    }

    void* existing_display_user_data = lv_display_get_user_data(lvgl_display);
    // esp_lvgl_port users user_data by default, so we have to modify the source
    // this is a check for when we upgrade esp_lvgl_port and forget to modify it again
    assert(existing_display_user_data == nullptr);
    lv_display_set_user_data(lvgl_display, display);

    lv_display_rotation_t rotation = app::display::getRotation();
    if (rotation != lv_disp_get_rotation(lv_disp_get_default())) {
        lv_disp_set_rotation(lv_disp_get_default(), static_cast<lv_display_rotation_t>(rotation));
    }

    return true;
}

bool initTouch(hal::Display* display, hal::Touch* touch) {
    TT_LOG_I(TAG, "Touch init");
    assert(display);
    assert(touch);
    if (touch->start(display->getLvglDisplay())) {
        return true;
    } else {
        TT_LOG_E(TAG, "Touch init failed");
        return false;
    }
}

bool initKeyboard(hal::Display* display, hal::Keyboard* keyboard) {
    TT_LOG_I(TAG, "Keyboard init");
    assert(display);
    assert(keyboard);
    if (keyboard->isAttached()) {
        if (keyboard->start(display->getLvglDisplay())) {
            lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
            lv_indev_set_user_data(keyboard_indev, keyboard);
            tt::lvgl::keypad_set_indev(keyboard_indev);
            TT_LOG_I(TAG, "Keyboard started");
            return true;
        } else {
            TT_LOG_E(TAG, "Keyboard start failed");
            return false;
        }
    } else {
        TT_LOG_E(TAG, "Keyboard attach failed");
        return false;
    }
}

void init(const hal::Configuration& config) {
    TT_LOG_I(TAG, "Starting");

    kernel::systemEventPublish(kernel::SystemEvent::BootInitLvglBegin);

    if (config.initLvgl != nullptr && !config.initLvgl()) {
        TT_LOG_E(TAG, "LVGL init failed");
        return;
    }

    if (!initDisplay(config)) {
        return;
    }

    hal::Display* display = getDisplay();

    hal::Touch* touch = display->createTouch();
    if (touch != nullptr) {
        initTouch(display, touch);
    }

    if (config.createKeyboard) {
        hal::Keyboard* keyboard = config.createKeyboard();
        initKeyboard(display, keyboard);
    }

    TT_LOG_I(TAG, "Finished");

    kernel::systemEventPublish(kernel::SystemEvent::BootInitLvglEnd);
}

} // namespace
