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

// ---- USB device-mode (peripheral) controller ----

/**
 * Which USB device-mode class currently owns the single TinyUSB device-mode slot.
 * Only one class may be active at a time - see usb_device_controller_claim().
 */
enum UsbDeviceClass {
    USB_DEVICE_CLASS_NONE,
    USB_DEVICE_CLASS_MSC,
    USB_DEVICE_CLASS_HID_KEYBOARD,
    USB_DEVICE_CLASS_MIDI,
};

/**
 * Shared owner of the TinyUSB device-mode (peripheral) stack. Exactly one USB device class
 * (mass storage, HID, ...) may be installed at a time - callers claim the slot before use and
 * release it when done. This exists so multiple independent drivers (MSC, HID, ...) can share
 * the single underlying `tinyusb_driver_install()` call and any board-specific PHY routing
 * without needing to know about each other.
 */
struct UsbDeviceControllerApi {
    /**
     * Claim the USB device-mode slot for the given class and install its descriptor.
     * @param[in] device the USB device controller device
     * @param[in] usb_class the class to claim the slot for
     * @param[in] class_config class-specific configuration, interpreted by the class's own driver
     * @retval ERROR_RESOURCE_BUSY if a different class already holds the slot
     * @retval ERROR_NONE on success
     */
    error_t (*claim)(struct Device* device, enum UsbDeviceClass usb_class, const void* class_config);

    /**
     * Release the USB device-mode slot. Disconnects from the host, uninstalls the TinyUSB
     * driver, and restores any board-specific PHY routing. Only the current holder may release.
     * @param[in] device the USB device controller device
     * @param[in] usb_class the class releasing the slot; must match the current holder
     * @retval ERROR_INVALID_STATE if usb_class does not hold the slot
     * @retval ERROR_NONE on success
     */
    error_t (*release)(struct Device* device, enum UsbDeviceClass usb_class);

    /**
     * @param[in] device the USB device controller device
     * @return the class currently holding the slot, or USB_DEVICE_CLASS_NONE if free
     */
    enum UsbDeviceClass (*get_active_class)(struct Device* device);
};

extern const struct DeviceType USB_DEVICE_CONTROLLER_TYPE;

/**
 * Find the first started USB device controller and take a reference on it.
 * @return the device with an outstanding reference, or NULL if none is available - caller must
 *         call device_put() exactly once when done, same as device_get_first_active_by_type().
 */
struct Device* usb_device_controller_get(void);

error_t usb_device_controller_claim(struct Device* device, enum UsbDeviceClass usb_class, const void* class_config);
error_t usb_device_controller_release(struct Device* device, enum UsbDeviceClass usb_class);
enum UsbDeviceClass usb_device_controller_get_active_class(struct Device* device);

#ifdef __cplusplus
}
#endif
