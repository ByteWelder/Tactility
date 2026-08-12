#include <tactility/drivers/usb_cdc_device.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_CDC_DEVICE_API(device) ((const struct UsbCdcDeviceApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_CDC_DEVICE_TYPE = {
    .name = "usb-cdc-device",
};

struct Device* usb_cdc_device_get() {
    struct Device* found = nullptr;
    device_get_first_active_by_type(&USB_CDC_DEVICE_TYPE, &found);
    return found;
}

bool usb_cdc_device_is_present(struct Device* device) {
    return USB_CDC_DEVICE_API(device)->is_present(device);
}

error_t usb_cdc_device_build_contribution(struct Device* device, struct Device* controller,
                                           uint8_t interface_string_index,
                                           struct UsbInterfaceContribution* out_contribution,
                                           const char** out_interface_string) {
    return USB_CDC_DEVICE_API(device)->build_contribution(device, controller, interface_string_index, out_contribution, out_interface_string);
}

error_t usb_cdc_device_start_console(struct Device* device) {
    return USB_CDC_DEVICE_API(device)->start_console(device);
}

error_t usb_cdc_device_stop_console(struct Device* device) {
    return USB_CDC_DEVICE_API(device)->stop_console(device);
}

} // extern "C"
