#include <Tactility/lvgl/KeyboardDeviceListener.h>
#include <Tactility/Mutex.h>

#include <tactility/device.h>
#include <tactility/device_listener.h>
#include <tactility/drivers/keyboard.h>
#include <tactility/log.h>

#include <lvgl/devices/keyboard.h>
#include <lvgl/lvgl.h>

#include <vector>

namespace tt::lvgl {

constexpr auto* TAG = "KeyboardDeviceListener";

namespace {

struct KeyboardBinding {
    Device* device;
    lv_indev_t* indev;
};

Mutex& bindingsMutex() {
    static Mutex mutex;
    return mutex;
}

std::vector<KeyboardBinding>& bindings() {
    static std::vector<KeyboardBinding> list;
    return list;
}

void onKeyboardDeviceStarted(Device* device) {
    lv_indev_t* indev = nullptr;
    lvgl_lock();
    error_t error = lvgl_keyboard_add(device, lv_display_get_default(), &indev);
    lvgl_unlock();
    if (error != ERROR_NONE) {
        LOG_E(TAG, "failed to bind keyboard device %s to LVGL", device->name);
        return;
    }

    auto lock = bindingsMutex().asScopedLock();
    lock.lock();
    bindings().push_back({ device, indev });
}

void onKeyboardDeviceStopped(Device* device) {
    lv_indev_t* indev = nullptr;
    {
        auto lock = bindingsMutex().asScopedLock();
        lock.lock();
        auto& list = bindings();
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->device == device) {
                indev = it->indev;
                list.erase(it);
                break;
            }
        }
    }
    if (indev == nullptr) {
        return;
    }

    lvgl_lock();
    lvgl_keyboard_remove(indev);
    lvgl_unlock();
}

void onDeviceEvent(Device* device, DeviceEvent event, void* context) {
    (void)context;
    if (device_get_type(device) != &KEYBOARD_TYPE) {
        return;
    }
    // Detach on STOPPING (before stop_device() frees the device's resources), not STOPPED (which
    // only fires after stop_device() already ran) - see DEVICE_EVENT_STOPPING's doc comment.
    if (event == DEVICE_EVENT_STARTED) {
        onKeyboardDeviceStarted(device);
    } else if (event == DEVICE_EVENT_STOPPING) {
        onKeyboardDeviceStopped(device);
    }
}

} // namespace

void startKeyboardDeviceListener() {
    device_listener_add(onDeviceEvent, nullptr);
}

void stopKeyboardDeviceListener() {
    device_listener_remove(onDeviceEvent);

    std::vector<KeyboardBinding> remaining;
    {
        auto lock = bindingsMutex().asScopedLock();
        lock.lock();
        remaining = std::move(bindings());
        bindings().clear();
    }

    if (remaining.empty()) {
        return;
    }

    lvgl_lock();
    for (auto& binding : remaining) {
        lvgl_keyboard_remove(binding.indev);
    }
    lvgl_unlock();
}

} // namespace tt::lvgl
