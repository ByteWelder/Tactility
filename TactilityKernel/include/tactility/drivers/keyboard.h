// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <tactility/device.h>
#include <tactility/error.h>

/**
 * @brief A single key event read from a keyboard device.
 */
struct KeyboardKeyData {
    /** @brief The key code. Driver-defined (e.g. ASCII/UTF-8 codepoint, scan code, or LVGL key code). */
    uint32_t key;
    /** @brief True if the key was pressed, false if released. */
    bool pressed;
    /**
     * @brief True if another key event is already queued and read_key() should be called again
     * immediately to drain it. False if this was the last pending event.
     */
    bool continue_reading;
    /**
     * @brief True if Ctrl was held when this key was pressed.
     *
     * Reported separately rather than folded into `key` because the two encodings collide: the C0
     * control codes a terminal expects for Ctrl chords (Ctrl+C is 0x03, Ctrl+K is 0x0B, ...) overlap
     * the LVGL key constants drivers emit in the same field (LV_KEY_END is 3, LV_KEY_PREV is 11,
     * LV_KEY_UP is 17, ...), so a single uint32_t cannot express both. Consumers that want control
     * codes derive them here, e.g.
     * `((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) ? (key & 0x1F) : key`
     * when ctrl is set.
     *
     * Drivers whose hardware cannot report Ctrl leave this false.
     */
    bool ctrl;
    /**
     * @brief True if Alt was held when this key was pressed. See ctrl for why modifiers are reported
     * separately. Drivers whose hardware cannot report Alt leave this false.
     */
    bool alt;
    /**
     * @brief Standard USB HID keyboard usage code for this key (USB HID Usage Tables, page 0x07),
     * or 0 if this driver doesn't compute one (most don't - `key` is the only field most consumers
     * need). Populated by drivers whose hardware layout maps cleanly onto HID usage codes, so
     * consumers that want to mirror physical key presses as real HID reports (e.g. USB HID output)
     * don't have to reverse-engineer one out of `key`'s LVGL/ASCII encoding - which is lossy for
     * keys with no ASCII/LVGL representation at all, e.g. F1-F12.
     */
    uint8_t hid_keycode;
    /**
     * @brief HID modifier bitmask (report byte 0: bit0=LeftCtrl, bit1=LeftShift, bit2=LeftAlt,
     * bit3=LeftGui, bit4-7=Right variants) matching hid_keycode, or 0 if hid_keycode is 0.
     */
    uint8_t hid_modifier;
};

/**
 * @brief API for keyboard drivers.
 */
struct KeyboardApi {
    /**
     * @brief Reads the next pending key event, if any.
     * @param[in] device the keyboard device
     * @param[out] data the key event data
     * @retval ERROR_NONE when the operation was successful
     */
    error_t (*read_key)(struct Device* device, struct KeyboardKeyData* data);

    /**
     * @brief Returns the baclight if the keyboard has one.
     * @warning Returns a referenced device. Must call device_put() afterwards.
     * @param[in] device the keyboard device
     * @param[out] backlight_device the output backlight device
     * @retval ERROR_NONE when the backlight_device was set
     * @retval ERROR_NOT_SUPPORTED when this device has no backlight
     */
    error_t (*get_backlight)(struct Device* device, struct Device** backlight_device);

    /**
     * @brief Optional: reports whether the keyboard is physically present right now. Only
     * meaningful for hot-pluggable/detachable keyboards (e.g. a removable accessory) whose
     * kernel device is constructed and started once at boot regardless of physical attachment -
     * leave NULL for a keyboard that's always physically present whenever its device is active
     * (the common case; callers must treat NULL the same as "always present").
     * @param[in] device the keyboard device
     * @return true if physically attached/present
     */
    bool (*is_present)(struct Device* device);
};

/**
 * @brief Reads the next pending key event using the specified keyboard device.
 */
error_t keyboard_read_key(struct Device* device, struct KeyboardKeyData* data);

/**
 * @brief Returns the backlight if the keyboard has one.
 * @warning Returns a referenced device. Must call device_put() afterwards.
 * @param[in] device the keyboard device
 * @param[out] backlight_device the output backlight device
 * @retval ERROR_NONE when the backlight_device was set
 * @retval ERROR_NOT_SUPPORTED when this device has no backlight
 */
error_t keyboard_get_backlight(struct Device* device, struct Device** backlight_device);

/**
 * @brief Whether the keyboard device is physically present right now. True when the driver
 * doesn't implement KeyboardApi::is_present (i.e. it's always physically present whenever its
 * device is active) - see that field's doc comment.
 * @param[in] device the keyboard device
 */
bool keyboard_is_present(struct Device* device);

extern const struct DeviceType KEYBOARD_TYPE;

#ifdef __cplusplus
}
#endif
