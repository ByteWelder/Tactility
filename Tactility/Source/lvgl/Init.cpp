#include "app/display/DisplayPreferences.h"
#include "lvgl.h"
#include "hal/Configuration.h"
#include "hal/Display.h"
#include "hal/Touch.h"
#include "hal/Keyboard.h"

namespace tt::lvgl {

#define TAG "lvglinit"

void init(const hal::Configuration& config) {
    TT_LOG_I(TAG, "Starting");

    if (config.initLvgl != nullptr && !config.initLvgl()) {
        TT_LOG_E(TAG, "LVGL init failed");
        return;
    }

    assert(config.createDisplay);
    auto* display = config.createDisplay();
    if (!display->start()) {
        TT_LOG_E(TAG, "Display start failed");
        return;
    }

    lv_display_t* lvgl_display = display->getLvglDisplay();
    tt_assert(lvgl_display);

    if (display->supportsBacklightDuty()) {
        display->setBacklightDuty(0);
    }

    void* existing_display_user_data = lv_display_get_user_data(lvgl_display);
    // esp_lvgl_port users user_data by default, so we have to modify the source
    // this is a check for when we upgrade esp_lvgl_port and forget to modify it again
    tt_assert(existing_display_user_data == nullptr);
    lv_display_set_user_data(lvgl_display, display);

    lv_display_rotation_t rotation = app::display::preferences_get_rotation();
    if (rotation != lv_disp_get_rotation(lv_disp_get_default())) {
        lv_disp_set_rotation(lv_disp_get_default(), static_cast<lv_display_rotation_t>(rotation));
    }

    hal::Touch* touch = display->getTouch();
    if (touch != nullptr) {
        if (touch->start(lvgl_display)) {
            TT_LOG_I(TAG, "Touch init");
        } else {
            TT_LOG_E(TAG, "Touch init failed");
        }
    }

    if (config.createKeyboard) {
        hal::Keyboard* keyboard = config.createKeyboard();
        if (keyboard->isAttached()) {
            if (keyboard->start(lvgl_display)) {
                lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
                lv_indev_set_user_data(keyboard_indev, keyboard);
                tt::lvgl::keypad_set_indev(keyboard_indev);
                TT_LOG_I(TAG, "Keyboard started");
            } else {
                TT_LOG_E(TAG, "Keyboard start failed");
            }
        } else {
            TT_LOG_E(TAG, "Keyboard attach failed");
        }
    }

    TT_LOG_I(TAG, "Finished");
}

} // namespace
