#include <tactility/drivers/usb_device_controller.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_DEVICE_CONTROLLER_API(device) ((const struct UsbDeviceControllerApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_DEVICE_CONTROLLER_TYPE = {
    .name = "usb-device-controller",
};

struct Device* usb_device_controller_get_device() {
    struct Device* found = nullptr;
    device_for_each_of_type(&USB_DEVICE_CONTROLLER_TYPE, &found, [](struct Device* dev, void* ctx) -> bool {
        if (device_is_ready(dev)) {
            *static_cast<struct Device**>(ctx) = dev;
            return false;
        }
        return true;
    });
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
