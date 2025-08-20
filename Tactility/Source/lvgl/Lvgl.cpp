#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/lvgl/Keyboard.h"
#include "Tactility/lvgl/Lvgl.h"

#include "Tactility/hal/display/DisplayDevice.h"
#include "Tactility/hal/touch/TouchDevice.h"
#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/keyboard/KeyboardDevice.h>
#include <Tactility/kernel/SystemEvents.h>

#ifdef ESP_PLATFORM
#include "Tactility/lvgl/EspLvglPort.h"
#endif

#include <lvgl.h>
#include <Tactility/TactilityHeadless.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/service/ServiceRegistration.h>

namespace tt::lvgl {

#define TAG "Lvgl"

static bool started = false;

static std::shared_ptr<hal::display::DisplayDevice> createDisplay(const hal::Configuration& config) {
    assert(config.createDisplay);
    auto display = config.createDisplay();
    assert(display != nullptr);

    if (!display->start()) {
        TT_LOG_E(TAG, "Display start failed");
        return nullptr;
    }

    if (display->supportsBacklightDuty()) {
        display->setBacklightDuty(0);
    }

    return display;
}

void init(const hal::Configuration& config) {
    TT_LOG_I(TAG, "Init started");

#ifdef ESP_PLATFORM
    if (config.lvglInit == hal::LvglInit::Default && !initEspLvglPort()) {
        return;
    }
#endif

    auto display = createDisplay(config);
    if (display == nullptr) {
        return;
    }
    hal::registerDevice(display);

    auto touch = display->getTouchDevice();
    if (touch != nullptr) {
        touch->start();
        hal::registerDevice(touch);
    }

    auto configuration = hal::getConfiguration();
    if (configuration->createKeyboard) {
        auto keyboard = configuration->createKeyboard();
        if (keyboard != nullptr) {
            hal::registerDevice(keyboard);
        }
    }

    start();

    TT_LOG_I(TAG, "Init finished");
}

bool isStarted() {
    return started;
}

void start() {
    TT_LOG_I(TAG, "Start LVGL");

    if (started) {
        TT_LOG_W(TAG, "Can't start LVGL twice");
        return;
    }

    auto lock = getSyncLock()->asScopedLock();
    lock.lock();

    // Start displays (their related touch devices start automatically within)

    auto displays = hal::findDevices<hal::display::DisplayDevice>(hal::Device::Type::Display);
    for (auto display : displays) {
        if (display->supportsLvgl() && display->startLvgl()) {
            auto lvgl_display = display->getLvglDisplay();
            assert(lvgl_display != nullptr);
            lv_display_rotation_t rotation = app::display::getRotation();
            if (rotation != lv_display_get_rotation(lvgl_display)) {
                lv_display_set_rotation(lvgl_display, rotation);
            }
        }
    }

    // Start touch

    auto touch_devices = hal::findDevices<hal::touch::TouchDevice>(hal::Device::Type::Touch);
    for (auto touch_device : touch_devices) {
        if (displays.size() > 0) {
            // TODO: Consider implementing support for multiple displays
            auto display = displays[0];
            // Start any touch devices that haven't been started yet
            if (touch_device->supportsLvgl() && touch_device->getLvglIndev() == nullptr) {
                touch_device->startLvgl(display->getLvglDisplay());
            }
        }
    }

    // Start keyboards

    auto keyboards = hal::findDevices<hal::keyboard::KeyboardDevice>(hal::Device::Type::Keyboard);
    for (auto keyboard : keyboards) {
        if (displays.size() > 0) {
            // TODO: Consider implementing support for multiple displays
            auto display = displays[0];
            if (keyboard->isAttached()) {
                if (keyboard->startLvgl(display->getLvglDisplay())) {
                    lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
                    hardware_keyboard_set_indev(keyboard_indev);
                    TT_LOG_I(TAG, "Keyboard started");
                } else {
                    TT_LOG_E(TAG, "Keyboard start failed");
                }
            }
        }
    }

    // Restart services

    if (service::getState("Gui") == service::State::Stopped) {
        service::startService("Gui");
    } else {
        TT_LOG_E(TAG, "Gui service is not in Stopped state");
    }

    if (service::getState("Statusbar") == service::State::Stopped) {
        service::startService("Statusbar");
    } else {
        TT_LOG_E(TAG, "Statusbar service is not in Stopped state");
    }

    // Finalize

    kernel::publishSystemEvent(kernel::SystemEvent::LvglStarted);

    started = true;
}

void stop() {
    TT_LOG_I(TAG, "Stop LVGL");

    if (!started) {
        TT_LOG_W(TAG, "Can't stop LVGL: not started");
        return;
    }

    auto lock = getSyncLock()->asScopedLock();
    lock.lock();

    // Stop services that highly depend on LVGL

    service::stopService("Statusbar");
    service::stopService("Gui");

    // Stop keyboards

    auto keyboards = hal::findDevices<hal::keyboard::KeyboardDevice>(hal::Device::Type::Keyboard);
    for (auto keyboard : keyboards) {
        if (keyboard->getLvglIndev() != nullptr) {
            keyboard->stopLvgl();
        }
    }

    // Stop touch

    // The display generally stops their own touch devices, but we'll clean up anything that didn't
    auto touch_devices = hal::findDevices<hal::touch::TouchDevice>(hal::Device::Type::Touch);
    for (auto touch_device : touch_devices) {
        if (touch_device->getLvglIndev() != nullptr) {
            touch_device->stopLvgl();
        }
    }

    // Stop displays (and their touch devices)

    auto displays = hal::findDevices<hal::display::DisplayDevice>(hal::Device::Type::Display);
    for (auto display : displays) {
        if (display->supportsLvgl() && display->getLvglDisplay() != nullptr && !display->stopLvgl()) {
            TT_LOG_E("HelloWorld", "Failed to detach display from LVGL");
        }
    }

    started = false;

    kernel::publishSystemEvent(kernel::SystemEvent::LvglStopped);
}

} // namespace
