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

// ---- USB MIDI device mode ----

/**
 * USB MIDI device profile API (present this device as a USB MIDI peripheral to a host).
 * This API is exposed by a child device of the USB device controller. Mirrors
 * bluetooth_midi.h's shape (no mode enum - unlike HID, MIDI has exactly one device profile) so
 * the two transports stay symmetric for callers that want to support both.
 */
struct UsbMidiDeviceApi {
    /**
     * Claim the USB device-mode slot and start advertising as a USB MIDI device.
     * @param[in] device the MIDI device child device
     * @retval ERROR_RESOURCE_BUSY if another USB device class already holds the slot
     * @retval ERROR_NONE on success
     */
    error_t (*start)(struct Device* device);

    /**
     * Stop presenting as a USB MIDI device and release the USB device-mode slot.
     * @param[in] device the MIDI device child device
     * @return ERROR_NONE on success
     */
    error_t (*stop)(struct Device* device);

    /**
     * Override the USB product name string reported to the host (iProduct descriptor). Only
     * takes effect on the next start() - matches bluetooth_set_device_name()'s pattern of being
     * set before starting advertising. If never called, falls back to "Tactility MIDI Device".
     * @param[in] device the MIDI device child device
     * @param[in] name the product name (copied; safe to free/reuse the caller's buffer after
     * this call returns)
     * @return ERROR_NONE on success
     */
    error_t (*set_name)(struct Device* device, const char* name);

    /**
     * Send raw MIDI message bytes over the USB MIDI connection.
     * @param[in] device the MIDI device child device
     * @param[in] msg the raw MIDI bytes
     * @param[in] len the number of bytes
     * @return ERROR_NONE on success
     */
    error_t (*send)(struct Device* device, const uint8_t* msg, size_t len);

    /**
     * @param[in] device the MIDI device child device
     * @return true when a USB host has mounted the device and the MIDI interface is ready
     */
    bool (*is_connected)(struct Device* device);
};

extern const struct DeviceType USB_MIDI_DEVICE_TYPE;

/**
 * Find the first started USB MIDI device child device and take a reference on it.
 * @return the device with an outstanding reference, or NULL if none is available - caller must
 *         call device_put() exactly once when done, same as device_get_first_active_by_type().
 */
struct Device* usb_midi_device_get(void);

error_t usb_midi_device_start(struct Device* device);
error_t usb_midi_device_stop(struct Device* device);
error_t usb_midi_device_set_name(struct Device* device, const char* name);
error_t usb_midi_device_send(struct Device* device, const uint8_t* msg, size_t len);
bool    usb_midi_device_is_connected(struct Device* device);

#ifdef __cplusplus
}
#endif
