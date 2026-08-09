#include <sdkconfig.h>
#if CONFIG_SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MIDI_COUNT

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/esp32_usbdevice.h>
#include <tactility/drivers/usb_device_controller.h>
#include <tactility/drivers/usb_midi_device.h>
#include <tactility/log.h>

#include <tinyusb.h>
#include <tusb.h>

#include <cstring>

#define TAG "esp32_usb_midi_device"
#define GET_CONFIG(device) ((const Esp32UsbDeviceChildConfig*)(device)->config)

// ---- MIDI device descriptor set ----
// TUD_MIDI_DESCRIPTOR consumes 2 interfaces internally (Audio Control + MIDI Streaming,
// itfnum and itfnum+1 - see TUD_MIDI_DESC_HEAD in usbd.h), unlike HID/MSC/CDC which use 1
// each - so ITF_NUM_TOTAL is 2 here even though this is a single logical MIDI port.

enum {
    ITF_NUM_MIDI_AC = 0, // Audio Control (required umbrella interface for MIDI v1.0)
    ITF_NUM_MIDI_MS,     // MIDI Streaming (the actual jacks/endpoints)
    ITF_NUM_TOTAL
};

enum {
    EDPT_MIDI_OUT = 0x01,
    EDPT_MIDI_IN  = 0x81,
};

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static const tusb_desc_device_t midi_device_descriptor = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4005,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

#if (TUD_OPT_HIGH_SPEED)
static const tusb_desc_device_qualifier_t midi_device_qualifier = {
    .bLength            = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00,
};
#endif

static const char midi_langid_descriptor[] = { 0x09, 0x04 };
// index 2 (iProduct) is overwritten by set_name() before start(), see
// midi_device_set_name()/midi_device_start(). Index 4 is the jack name (see
// midi_configuration_descriptor below).
//
// The interface string index is literal 0 ("no string" - a real, well-defined USB concept: the
// host skips string lookup entirely, distinct from an empty string AT a valid index), not an
// index into this table at all. An earlier attempt used an empty "" entry at a real index
// instead of literal 0 - that crashed the device outright on Windows (Code 10,
// STATUS_DEVICE_DATA_ERROR): empty-string-at-a-valid-index and "no string" (index 0) are not
// the same thing, only the latter is safe. Both Windows' newer MIDI naming and MIDIBerry were
// confirmed (live test) to concatenate interface-string + jack-string into one displayed name,
// so leaving the interface string as "no string" (0) means only the jack string shows, instead
// of duplicating/prefixing the product name.
static char midi_product_string[32] = "Tactility MIDI Device";
static const char* midi_string_descriptor[] = {
    midi_langid_descriptor, "Tactility", midi_product_string, "123456", midi_product_string,
};
static constexpr uint8_t MIDI_STRIDX_JACK = 4;

// TUD_MIDI_DESCRIPTOR (the usbd.h convenience macro) hardcodes its jack descriptors' string
// index to 0 (no string) - it isn't a parameter the macro exposes. Fixed by expanding
// TUD_MIDI_DESCRIPTOR's own definition here with a real string index for the jack (instead of
// the macro's hardcoded 0), while keeping the interface string index at literal 0 (see comment
// above on midi_string_descriptor for why 0, not an empty table entry).
static const uint8_t midi_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MIDI_DESC_HEAD(ITF_NUM_MIDI_AC, 0, 1),
    TUD_MIDI_DESC_JACK_DESC(1, MIDI_STRIDX_JACK),
    TUD_MIDI_DESC_EP(EDPT_MIDI_OUT, 64, 1),
    TUD_MIDI_JACKID_IN_EMB(1),
    TUD_MIDI_DESC_EP(EDPT_MIDI_IN, 64, 1),
    TUD_MIDI_JACKID_OUT_EMB(1),
};

static const tinyusb_config_t midi_tusb_cfg = {
    .device_descriptor = &midi_device_descriptor,
    .string_descriptor = midi_string_descriptor,
    .string_descriptor_count = sizeof(midi_string_descriptor) / sizeof(midi_string_descriptor[0]),
    .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = midi_configuration_descriptor,
    .hs_configuration_descriptor = midi_configuration_descriptor,
    .qualifier_descriptor = &midi_device_qualifier,
#else
    .configuration_descriptor = midi_configuration_descriptor,
#endif
    .self_powered = false,
    .vbus_monitor_io = 0,
};

// ---- MIDI device API ----

static error_t midi_device_start(struct Device* device) {
    auto* controller = device_get_parent(device);
    return usb_device_controller_claim(controller, USB_DEVICE_CLASS_MIDI, &midi_tusb_cfg);
}

static error_t midi_device_stop(struct Device* device) {
    auto* controller = device_get_parent(device);
    return usb_device_controller_release(controller, USB_DEVICE_CLASS_MIDI);
}

static error_t midi_device_set_name(struct Device* device, const char* name) {
    (void)device;
    if (name == nullptr) {
        strncpy(midi_product_string, "Tactility MIDI Device", sizeof(midi_product_string) - 1);
        midi_product_string[sizeof(midi_product_string) - 1] = '\0';
        return ERROR_NONE;
    }
    strncpy(midi_product_string, name, sizeof(midi_product_string) - 1);
    midi_product_string[sizeof(midi_product_string) - 1] = '\0';
    return ERROR_NONE;
}

static error_t midi_device_send(struct Device* device, const uint8_t* msg, size_t len) {
    (void)device;
    if (!tud_mounted()) {
        return ERROR_INVALID_STATE;
    }
    uint32_t written = tud_midi_stream_write(0, msg, static_cast<uint32_t>(len));
    return written == len ? ERROR_NONE : ERROR_RESOURCE;
}

static bool midi_device_is_connected(struct Device* device) {
    (void)device;
    return tud_mounted();
}

extern const UsbMidiDeviceApi esp32_usb_midi_device_api = {
    .start        = midi_device_start,
    .stop         = midi_device_stop,
    .set_name     = midi_device_set_name,
    .send         = midi_device_send,
    .is_connected = midi_device_is_connected,
};

// ---- Driver lifecycle ----
// Defined in each board's .dts as a child of usbdevice0 (e.g. usbdevicemidi0) - the devicetree
// compiler wires device_get_parent() to the controller automatically.

extern "C" {

// tud_midi_rx_cb is intentionally not implemented: this driver is send-only, matching
// bluetooth_midi.h's API shape (no receive exposed at the Tactility layer either). Incoming
// bytes are simply left in TinyUSB's RX FIFO and eventually overwritten/dropped.

static error_t start_device(struct Device* device) {
    (void)GET_CONFIG(device); // no configuration - placeholder only
    return ERROR_NONE;
}

static error_t stop_device(struct Device* device) {
    // Safety cleanup: release the slot if it's still held (e.g. this device is stopped while
    // MIDI is still active).
    auto* controller = device_get_parent(device);
    if (controller != nullptr && usb_device_controller_get_active_class(controller) == USB_DEVICE_CLASS_MIDI) {
        usb_device_controller_release(controller, USB_DEVICE_CLASS_MIDI);
    }
    return ERROR_NONE;
}

Driver esp32_usb_midi_device_driver = {
    .name         = "esp32_usb_midi_device",
    .compatible   = (const char*[]) { "espressif,esp32-usbdevice-midi", nullptr },
    .start_device = start_device,
    .stop_device  = stop_device,
    .api          = &esp32_usb_midi_device_api,
    .device_type  = &USB_MIDI_DEVICE_TYPE,
    .owner        = nullptr,
    .internal     = nullptr,
};

} // extern "C"

#endif // CONFIG_SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MIDI_COUNT
