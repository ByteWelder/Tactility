// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Device;
struct DeviceType;

// ---- USB HID device mode ----

/**
 * Selects the HID report descriptor installed when this device operates as a USB HID
 * peripheral. Mirrors BtHidDeviceMode's shape (bluetooth_hid_device.h) so callers that want to
 * support both transports use the same mode taxonomy and send_* call shape.
 */
enum UsbHidDeviceMode {
    /** Keyboard (report ID 1, boot-protocol-compatible 8 bytes) + Consumer (report ID 2, 2 bytes). */
    USB_HID_DEVICE_MODE_KEYBOARD,
    /** Mouse only (report ID 1, 4 bytes). */
    USB_HID_DEVICE_MODE_MOUSE,
    /** Keyboard + Consumer + Mouse (report IDs 1, 2, 3). */
    USB_HID_DEVICE_MODE_KEYBOARD_MOUSE,
    /** Gamepad (report ID 1, 8 bytes: 5-byte axes, 1-byte hat/dpad, 2-byte buttons[10] padded). */
    USB_HID_DEVICE_MODE_GAMEPAD,
};

/**
 * USB HID device profile API (present this device as a USB HID peripheral to a host).
 * This API is exposed by a child device of the USB device controller.
 */
struct UsbHidDeviceApi {
    /**
     * Claim the USB device-mode slot and start advertising as a USB HID device with the given
     * mode.
     * @param[in] device the HID device child device
     * @param[in] mode the HID device mode (keyboard, mouse, keyboard+mouse, gamepad)
     * @retval ERROR_RESOURCE_BUSY if another USB device class already holds the slot
     * @retval ERROR_NONE on success
     */
    error_t (*start)(struct Device* device, enum UsbHidDeviceMode mode);

    /**
     * Stop presenting as a USB HID device and release the USB device-mode slot.
     * @param[in] device the HID device child device
     * @return ERROR_NONE on success
     */
    error_t (*stop)(struct Device* device);

    /**
     * Override the USB product name string reported to the host (iProduct descriptor). Only
     * takes effect on the next start() - the descriptor is fixed for the lifetime of a claimed
     * session, matching bluetooth_set_device_name()'s pattern of being set before starting
     * advertising. If never called, each mode falls back to its own default name (e.g.
     * "Tactility Keyboard", "Tactility Mouse").
     * @param[in] device the HID device child device
     * @param[in] name the product name (copied; safe to free/reuse the caller's buffer after
     * this call returns)
     * @return ERROR_NONE on success
     */
    error_t (*set_name)(struct Device* device, const char* name);

    /**
     * Send a keyboard HID report (report ID 1: modifier, reserved, keycodes[6] - 8 bytes after
     * the ID byte). Only valid in USB_HID_DEVICE_MODE_KEYBOARD or _KEYBOARD_MOUSE.
     * @param[in] device the HID device child device
     * @param[in] report pointer to the 8-byte keyboard report (modifier, reserved, keycode[6])
     * @param[in] len number of bytes (up to 8)
     * @return ERROR_NONE on success
     */
    error_t (*send_keyboard)(struct Device* device, const uint8_t* report, size_t len);

    /**
     * Send a consumer control HID report (report ID 2: 16-bit usage code, little-endian).
     * Only valid in USB_HID_DEVICE_MODE_KEYBOARD or _KEYBOARD_MOUSE.
     * @param[in] device the HID device child device
     * @param[in] report pointer to the 2-byte consumer report
     * @param[in] len number of bytes (up to 2)
     * @return ERROR_NONE on success
     */
    error_t (*send_consumer)(struct Device* device, const uint8_t* report, size_t len);

    /**
     * Send a mouse HID report (buttons, X, Y, wheel - 4 bytes). Report ID 1 in
     * USB_HID_DEVICE_MODE_MOUSE, report ID 3 in _KEYBOARD_MOUSE.
     * @param[in] device the HID device child device
     * @param[in] report pointer to the 4-byte mouse report
     * @param[in] len number of bytes (up to 4)
     * @return ERROR_NONE on success
     */
    error_t (*send_mouse)(struct Device* device, const uint8_t* report, size_t len);

    /**
     * Send a gamepad HID report (report ID 1: axes[5] (X,Y,Rx,Ry,Z), hat/dpad[1] (low nibble),
     * buttons[2] (10 buttons + 6 padding bits) - 8 bytes). Only valid in
     * USB_HID_DEVICE_MODE_GAMEPAD. See hid_report_map_gamepad in hid_report_descriptors.cpp for
     * the full wire layout.
     * @param[in] device the HID device child device
     * @param[in] report pointer to the 8-byte gamepad report
     * @param[in] len number of bytes (up to 8)
     * @return ERROR_NONE on success
     */
    error_t (*send_gamepad)(struct Device* device, const uint8_t* report, size_t len);

    /**
     * @param[in] device the HID device child device
     * @return true when a USB host has mounted the device and the HID interface is ready
     */
    bool (*is_connected)(struct Device* device);
};

extern const struct DeviceType USB_HID_DEVICE_TYPE;

/**
 * Find the first started USB HID device child device and take a reference on it.
 * @return the device with an outstanding reference, or NULL if none is available - caller must
 *         call device_put() exactly once when done, same as device_get_first_active_by_type().
 */
struct Device* usb_hid_device_get(void);

error_t usb_hid_device_start(struct Device* device, enum UsbHidDeviceMode mode);
error_t usb_hid_device_stop(struct Device* device);
error_t usb_hid_device_set_name(struct Device* device, const char* name);
error_t usb_hid_device_send_keyboard(struct Device* device, const uint8_t* report, size_t len);
error_t usb_hid_device_send_consumer(struct Device* device, const uint8_t* report, size_t len);
error_t usb_hid_device_send_mouse(struct Device* device, const uint8_t* report, size_t len);
error_t usb_hid_device_send_gamepad(struct Device* device, const uint8_t* report, size_t len);
bool    usb_hid_device_is_connected(struct Device* device);

#ifdef __cplusplus
}
#endif
