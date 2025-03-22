#include "../../Tactility/app/display/DisplaySettings.h"
#include "../../Tactility/lvgl/Keyboard.h"
#include "../../Tactility/hal/display/DisplayDevice.h"
#include "../../Tactility/hal/touch/TouchDevice.h"
#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/keyboard/KeyboardDevice.h>
#include <Tactility/kernel/SystemEvents.h>

#ifdef ESP_PLATFORM
#include "Tactility/lvgl/EspLvglPort.h"
#endif

#include <lvgl.h>

namespace tt::lvgl {

#define TAG "cyd22_init"

static std::shared_ptr<tt::hal::display::DisplayDevice> initDisplay(const hal::Configuration& config) {
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
    assert(existing_display_user_data == nullptr);
    lv_display_set_user_data(lvgl_display, display.get());

    lv_display_rotation_t rotation = app::display::getRotation();
    lv_display_rotation_t current_rotation = lv_disp_get_rotation(lvgl_display);
    TT_LOG_I(TAG, "Current rotation: %d, NVS rotation: %d", current_rotation, rotation);
    if (rotation != current_rotation && rotation != LV_DISPLAY_ROTATION_0) {  // Only override if NVS has a non-default value
        TT_LOG_I(TAG, "Overriding rotation to %d from NVS", rotation);
        lv_disp_set_rotation(lvgl_display, rotation);
    }

    return display;
}

static bool initTouch(const std::shared_ptr<hal::display::DisplayDevice>& display, const std::shared_ptr<hal::touch::TouchDevice>& touch) {
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

static bool initKeyboard(const std::shared_ptr<hal::display::DisplayDevice>& display, const std::shared_ptr<hal::keyboard::KeyboardDevice>& keyboard) {
    TT_LOG_I(TAG, "Keyboard init");
    assert(display);
    assert(keyboard);
    if (keyboard->isAttached()) {
        if (keyboard->start(display->getLvglDisplay())) {
            lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
            lv_indev_set_user_data(keyboard_indev, keyboard.get());
            tt::lvgl::hardware_keyboard_set_indev(keyboard_indev);
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

#ifdef ESP_PLATFORM
    if (config.lvglInit == hal::LvglInit::Default && !initEspLvglPort()) {
        return;
    }
#endif

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
