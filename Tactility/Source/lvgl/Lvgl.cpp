#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/encoder/EncoderDevice.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/keyboard/KeyboardDevice.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <Tactility/lvgl/Keyboard.h>
#include <Tactility/lvgl/Lvgl.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/kernel/SystemEvents.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/settings/DisplaySettings.h>

#ifdef ESP_PLATFORM
#include <Tactility/lvgl/EspLvglPort.h>
#endif

#include <lvgl.h>

namespace tt::lvgl {

constexpr auto* TAG = "Lvgl";

static bool started = false;

void init(const hal::Configuration& config) {
    TT_LOG_I(TAG, "Init started");

#ifdef ESP_PLATFORM
    if (config.lvglInit == hal::LvglInit::Default && !initEspLvglPort()) {
        return;
    }
#endif

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

    TT_LOG_I(TAG, "Start displays");
    auto displays = hal::findDevices<hal::display::DisplayDevice>(hal::Device::Type::Display);
    for (auto display : displays) {
        if (display->supportsLvgl()) {
            if (display->startLvgl()) {
                TT_LOG_I(TAG, "Started %s", display->getName().c_str());
                auto lvgl_display = display->getLvglDisplay();
                assert(lvgl_display != nullptr);
                auto settings = settings::display::loadOrGetDefault();
                lv_display_rotation_t rotation = settings::display::toLvglDisplayRotation(settings.orientation);
                if (rotation != lv_display_get_rotation(lvgl_display)) {
                    lv_display_set_rotation(lvgl_display, rotation);
                }
            } else {
                TT_LOG_E(TAG, "Start failed for %s", display->getName().c_str());
            }
        }
    }

    // Start touch

    // TODO: Consider implementing support for multiple displays
    auto primary_display = !displays.empty() ? displays[0] : nullptr;

    // Start display-related peripherals
    if (primary_display != nullptr) {
        TT_LOG_I(TAG, "Start touch devices");
        auto touch_devices = hal::findDevices<hal::touch::TouchDevice>(hal::Device::Type::Touch);
        for (auto touch_device : touch_devices) {
            // Start any touch devices that haven't been started yet
            if (touch_device->supportsLvgl() && touch_device->getLvglIndev() == nullptr) {
                if (touch_device->startLvgl(primary_display->getLvglDisplay())) {
                    TT_LOG_I(TAG, "Started %s", touch_device->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "Start failed for %s", touch_device->getName().c_str());
                }
            }
        }

        // Start keyboards
        TT_LOG_I(TAG, "Start keyboards");
        auto keyboards = hal::findDevices<hal::keyboard::KeyboardDevice>(hal::Device::Type::Keyboard);
        for (auto keyboard : keyboards) {
            if (keyboard->isAttached()) {
                if (keyboard->startLvgl(primary_display->getLvglDisplay())) {
                    lv_indev_t* keyboard_indev = keyboard->getLvglIndev();
                    hardware_keyboard_set_indev(keyboard_indev);
                    TT_LOG_I(TAG, "Started %s", keyboard->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "Start failed for %s", keyboard->getName().c_str());
                }
            }
        }

        // Start encoders
        TT_LOG_I(TAG, "Start encoders");
        auto encoders = hal::findDevices<hal::encoder::EncoderDevice>(hal::Device::Type::Encoder);
        for (auto encoder : encoders) {
            if (encoder->startLvgl(primary_display->getLvglDisplay())) {
                TT_LOG_I(TAG, "Started %s", encoder->getName().c_str());
            } else {
                TT_LOG_E(TAG, "Start failed for %s", encoder->getName().c_str());
            }
        }
    }

    // Restart services

    // We search for the manifest first, because during the initial start() during boot
    // the service won't be registered yet.
    if (service::findManifestById("Gui") != nullptr) {
        if (service::getState("Gui") == service::State::Stopped) {
            service::startService("Gui");
        } else {
            TT_LOG_E(TAG, "Gui service is not in Stopped state");
        }
    }

    // We search for the manifest first, because during the initial start() during boot
    // the service won't be registered yet.
    if (service::findManifestById("Statusbar") != nullptr) {
        if (service::getState("Statusbar") == service::State::Stopped) {
            service::startService("Statusbar");
        } else {
            TT_LOG_E(TAG, "Statusbar service is not in Stopped state");
        }
    }

    // Finalize

    kernel::publishSystemEvent(kernel::SystemEvent::LvglStarted);

    started = true;
}

void stop() {
    TT_LOG_I(TAG, "Stopping LVGL");

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

    TT_LOG_I(TAG, "Stopping keyboards");
    auto keyboards = hal::findDevices<hal::keyboard::KeyboardDevice>(hal::Device::Type::Keyboard);
    for (auto keyboard : keyboards) {
        if (keyboard->getLvglIndev() != nullptr) {
            keyboard->stopLvgl();
        }
    }

    // Stop touch

    TT_LOG_I(TAG, "Stopping touch");
    // The display generally stops their own touch devices, but we'll clean up anything that didn't
    auto touch_devices = hal::findDevices<hal::touch::TouchDevice>(hal::Device::Type::Touch);
    for (auto touch_device : touch_devices) {
        if (touch_device->getLvglIndev() != nullptr) {
            touch_device->stopLvgl();
        }
    }

    // Stop encoders

    TT_LOG_I(TAG, "Stopping encoders");
    // The display generally stops their own touch devices, but we'll clean up anything that didn't
    auto encoder_devices = hal::findDevices<hal::encoder::EncoderDevice>(hal::Device::Type::Encoder);
    for (auto encoder_device : encoder_devices) {
        if (encoder_device->getLvglIndev() != nullptr) {
            encoder_device->stopLvgl();
        }
    }
    // Stop displays (and their touch devices)

    TT_LOG_I(TAG, "Stopping displays");
    auto displays = hal::findDevices<hal::display::DisplayDevice>(hal::Device::Type::Display);
    for (auto display : displays) {
        if (display->supportsLvgl() && display->getLvglDisplay() != nullptr && !display->stopLvgl()) {
            TT_LOG_E("HelloWorld", "Failed to detach display from LVGL");
        }
    }

    started = false;

    kernel::publishSystemEvent(kernel::SystemEvent::LvglStopped);

    TT_LOG_I(TAG, "Stopped LVGL");
}

} // namespace
