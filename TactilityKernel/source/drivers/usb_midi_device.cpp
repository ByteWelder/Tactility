#include <tactility/drivers/usb_midi_device.h>
#include <tactility/device.h>
#include <tactility/driver.h>

#define USB_MIDI_DEVICE_API(device) ((const struct UsbMidiDeviceApi*)device_get_driver(device)->api)

extern "C" {

const struct DeviceType USB_MIDI_DEVICE_TYPE = {
    .name = "usb-midi-device",
};

struct Device* usb_midi_device_get() {
    struct Device* found = nullptr;
    device_get_first_active_by_type(&USB_MIDI_DEVICE_TYPE, &found);
    return found;
}

error_t usb_midi_device_start(struct Device* device) {
    return USB_MIDI_DEVICE_API(device)->start(device);
}

error_t usb_midi_device_stop(struct Device* device) {
    return USB_MIDI_DEVICE_API(device)->stop(device);
}

error_t usb_midi_device_set_name(struct Device* device, const char* name) {
    return USB_MIDI_DEVICE_API(device)->set_name(device, name);
}

error_t usb_midi_device_send(struct Device* device, const uint8_t* msg, size_t len) {
    return USB_MIDI_DEVICE_API(device)->send(device, msg, len);
}

bool usb_midi_device_is_connected(struct Device* device) {
    return USB_MIDI_DEVICE_API(device)->is_connected(device);
}

} // extern "C"
