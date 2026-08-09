#include <tactility/drivers/usb_msc_device.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_MSC_DEVICE_API(device) ((const struct UsbMscDeviceApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_MSC_DEVICE_TYPE = {
    .name = "usb-msc-device",
};

struct Device* usb_msc_device_get_device() {
    struct Device* found = nullptr;
    device_for_each_of_type(&USB_MSC_DEVICE_TYPE, &found, [](struct Device* dev, void* ctx) -> bool {
        if (device_is_ready(dev)) {
            *static_cast<struct Device**>(ctx) = dev;
            return false;
        }
        return true;
    });
    return found;
}

error_t usb_msc_device_start(struct Device* device, enum UsbMscDeviceSource source, void* source_handle,
                             UsbMscDeviceMountChangedCallback mount_changed_cb, void* context) {
    return USB_MSC_DEVICE_API(device)->start(device, source, source_handle, mount_changed_cb, context);
}

error_t usb_msc_device_stop(struct Device* device) {
    return USB_MSC_DEVICE_API(device)->stop(device);
}

bool usb_msc_device_is_connected(struct Device* device) {
    return USB_MSC_DEVICE_API(device)->is_connected(device);
}

} // extern "C"
