#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/lvgl/LvglKeypad.h"
#include "Tactility/lvgl/LvglDisplay.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/Keyboard.h>
#include <Tactility/hal/Touch.h>
#include <Tactility/kernel/SystemEvents.h>

#include <lvgl.h>

namespace tt::lvgl {

#define TAG "lvgl_init"

static std::shared_ptr<tt::hal::Display> initDisplay(const hal::Configuration& config) {
    assert(config.createDisplay);
    auto display = config.createDisplay();
    assert(display != nullptr);

    if (!display->start()) {
        TT_LOG_E(TAG, "Display start failed");
        return nullptr;
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
    lv_display_set_user_data(lvgl_display, display.get());

    lv_display_rotation_t rotation = app::display::getRotation();
    if (rotation != lv_disp_get_rotation(lv_disp_get_default())) {
        lv_disp_set_rotation(lv_disp_get_default(), static_cast<lv_display_rotation_t>(rotation));
    }

    return display;
}

static bool initTouch(const std::shared_ptr<hal::Display>& display, const std::shared_ptr<hal::Touch>& touch) {
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

static bool initKeyboard(const std::shared_ptr<hal::Display>& display, const std::shared_ptr<hal::Keyboard>& keyboard) {
    TT_LOG_I(TAG, "Keyboard init");
    assert(display);
    assert(keyboard);
    if (keyboard->isAttached()) {
        if (keyboard->start(display->getLvglDisplay())) {
            lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
            lv_indev_set_user_data(keyboard_indev, keyboard.get());
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

    auto display = initDisplay(config);
    if (display == nullptr) {
        return;
    }
    hal::registerDevice(display);

    auto touch = display->createTouch();
    if (touch != nullptr) {
        hal::registerDevice(touch);
        initTouch(display, touch);
    }

    if (config.createKeyboard) {
        auto keyboard = config.createKeyboard();
        if (keyboard != nullptr) {
            hal::registerDevice(keyboard);
            initKeyboard(display, keyboard);
        }
    }

    TT_LOG_I(TAG, "Finished");

    kernel::systemEventPublish(kernel::SystemEvent::BootInitLvglEnd);
}

} // namespace
