#include <tactility/drivers/usb_hid_device.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_HID_DEVICE_API(device) ((const struct UsbHidDeviceApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_HID_DEVICE_TYPE = {
    .name = "usb-hid-device",
};

struct Device* usb_hid_device_get() {
    struct Device* found = nullptr;
    device_get_first_active_by_type(&USB_HID_DEVICE_TYPE, &found);
    return found;
}

error_t usb_hid_device_start(struct Device* device, enum UsbHidDeviceMode mode) {
    return USB_HID_DEVICE_API(device)->start(device, mode);
}

error_t usb_hid_device_stop(struct Device* device) {
    return USB_HID_DEVICE_API(device)->stop(device);
}

error_t usb_hid_device_set_name(struct Device* device, const char* name) {
    return USB_HID_DEVICE_API(device)->set_name(device, name);
}

error_t usb_hid_device_send_keyboard(struct Device* device, const uint8_t* report, size_t len) {
    return USB_HID_DEVICE_API(device)->send_keyboard(device, report, len);
}

error_t usb_hid_device_send_consumer(struct Device* device, const uint8_t* report, size_t len) {
    return USB_HID_DEVICE_API(device)->send_consumer(device, report, len);
}

error_t usb_hid_device_send_mouse(struct Device* device, const uint8_t* report, size_t len) {
    return USB_HID_DEVICE_API(device)->send_mouse(device, report, len);
}

error_t usb_hid_device_send_gamepad(struct Device* device, const uint8_t* report, size_t len) {
    return USB_HID_DEVICE_API(device)->send_gamepad(device, report, len);
}

bool usb_hid_device_is_connected(struct Device* device) {
    return USB_HID_DEVICE_API(device)->is_connected(device);
}

} // extern "C"
