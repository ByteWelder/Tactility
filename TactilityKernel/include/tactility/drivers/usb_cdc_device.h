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
struct UsbInterfaceContribution;

// ---- USB device-mode CDC-ACM addon ----

/**
 * USB CDC-ACM addon API (present a serial console over USB device mode). Unlike MSC/HID/MIDI,
 * CDC is not a primary USB device class and does not go through
 * usb_device_controller_claim()/release() - it's a devicetree-presence addon the USB device
 * controller composites into whichever primary class is active (or stands alone with none
 * active), toggled per-board via the usbdevicecdc0 devicetree child's status. This API is
 * exposed by that child device and called by the USB device controller itself, not by primary
 * class drivers.
 */
struct UsbCdcDeviceApi {
    /**
     * @param[in] device the CDC child device
     * @return true if this board's usbdevicecdc0 child is present and enabled (status != "disabled")
     */
    bool (*is_present)(struct Device* device);

    /**
     * Build this CDC instance's descriptor contribution, requesting interface/endpoint numbers
     * from the controller via usb_device_controller_allocate_interfaces(). Called by the USB
     * device controller itself during claim(), after the primary class's own contribution has
     * already been allocated.
     *
     * `interface_string_index` is the string-descriptor index CDC's own interface string will
     * land at once the controller appends it to the primary's string table (always
     * primary_string_descriptor_count, since CDC's string is always appended last) - CDC bakes
     * this index into its TUD_CDC_DESCRIPTOR bytes directly since it can't be patched after the
     * fact. `out_interface_string` is CDC's own interface string text for the controller to
     * append at that same index.
     *
     * @param[in] device the CDC child device
     * @param[in] controller the USB device controller (to call allocate_interfaces() on)
     * @param[in] interface_string_index string index this contribution's descriptor bytes must reference
     * @param[out] out_contribution the built fragment
     * @param[out] out_interface_string CDC's interface string text, to append at interface_string_index
     * @retval ERROR_NONE on success
     */
    error_t (*build_contribution)(struct Device* device, struct Device* controller,
                                   uint8_t interface_string_index,
                                   struct UsbInterfaceContribution* out_contribution,
                                   const char** out_interface_string);

    /**
     * Install the CDC ACM interface and the log-mirroring vprintf hook. Called by the USB device
     * controller after claim() successfully installs the composite descriptor.
     * @param[in] device the CDC child device
     * @retval ERROR_NONE on success
     */
    error_t (*start_console)(struct Device* device);

    /**
     * Uninstall the CDC ACM interface and restore the previous vprintf hook. Called by the USB
     * device controller before release() uninstalls the TinyUSB driver.
     * @param[in] device the CDC child device
     * @retval ERROR_NONE on success
     */
    error_t (*stop_console)(struct Device* device);
};

extern const struct DeviceType USB_CDC_DEVICE_TYPE;

/**
 * Find the first started USB CDC device and take a reference on it.
 * @return the device with an outstanding reference, or NULL if none is available - caller must
 *         call device_put() exactly once when done, same as device_get_first_active_by_type().
 */
struct Device* usb_cdc_device_get(void);

bool usb_cdc_device_is_present(struct Device* device);
error_t usb_cdc_device_build_contribution(struct Device* device, struct Device* controller,
                                           uint8_t interface_string_index,
                                           struct UsbInterfaceContribution* out_contribution,
                                           const char** out_interface_string);
error_t usb_cdc_device_start_console(struct Device* device);
error_t usb_cdc_device_stop_console(struct Device* device);

#ifdef __cplusplus
}
#endif
