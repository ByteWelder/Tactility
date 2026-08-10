#include <tactility/drivers/usb_device_controller.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_DEVICE_CONTROLLER_API(device) ((const struct UsbDeviceControllerApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_DEVICE_CONTROLLER_TYPE = {
    .name = "usb-device-controller",
};

struct Device* usb_device_controller_get() {
    struct Device* found = nullptr;
    device_get_first_active_by_type(&USB_DEVICE_CONTROLLER_TYPE, &found);
    return found;
}

error_t usb_device_controller_claim(struct Device* device, enum UsbDeviceClass usb_class, const void* class_config) {
    return USB_DEVICE_CONTROLLER_API(device)->claim(device, usb_class, class_config);
}

error_t usb_device_controller_release(struct Device* device, enum UsbDeviceClass usb_class) {
    return USB_DEVICE_CONTROLLER_API(device)->release(device, usb_class);
}

enum UsbDeviceClass usb_device_controller_get_active_class(struct Device* device) {
    return USB_DEVICE_CONTROLLER_API(device)->get_active_class(device);
}

} // extern "C"
