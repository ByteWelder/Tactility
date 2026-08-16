#pragma once

namespace tt::lvgl {

/**
 * @brief Starts listening for KEYBOARD_TYPE kernel devices being started/stopped at runtime
 * (e.g. a USB HID keyboard being plugged in/out) and binds/unbinds each one to LVGL via
 * lvgl_keyboard_add()/lvgl_keyboard_remove().
 *
 * Devices already started before this is called are picked up by lvgl-module's own boot-time
 * scan instead (see Modules/lvgl-module/source/devices/devices.cpp's lvgl_devices_attach()) -
 * this only reacts to devices that start/stop afterwards.
 */
void startKeyboardDeviceListener();

/**
 * @brief Stops listening and unbinds every KEYBOARD_TYPE device this listener bound to LVGL.
 */
void stopKeyboardDeviceListener();

} // namespace tt::lvgl
