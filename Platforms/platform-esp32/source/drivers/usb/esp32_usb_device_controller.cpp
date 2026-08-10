#include <sdkconfig.h>
#if CONFIG_SOC_USB_OTG_SUPPORTED && (CONFIG_TINYUSB_HID_COUNT || CONFIG_TINYUSB_MSC_ENABLED || CONFIG_TINYUSB_MIDI_COUNT)

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/esp32_usbdevice.h>
#include <tactility/drivers/usb_device_controller.h>
#include <tactility/log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <tinyusb.h>
#include <tusb.h>

#if CONFIG_IDF_TARGET_ESP32P4
#include <hal/usb_wrap_ll.h>
#include <soc/usb_wrap_struct.h>
#endif

#define TAG "esp32_usb_device_controller"
#define GET_CONFIG(device) ((const Esp32UsbDeviceConfig*)(device)->config)

// ---- Controller state ----
// One TinyUSB device-mode slot, shared by every USB device class (each a separate child device
// under usbdevice0 in the devicetree - see esp32_usb_device_hid.cpp / esp32_usb_device_msc.cpp).
// Only one class may be installed at a time - see UsbDeviceControllerApi::claim().

struct UsbDeviceControllerCtx {
    enum UsbDeviceClass active_class = USB_DEVICE_CLASS_NONE;
    bool phy_routed = false;
};

static void route_phy_for_device_mode() {
#if CONFIG_IDF_TARGET_ESP32P4
    // Tab5's USB-C is wired to ESP32-P4 FSLS PHY0 (GPIO24/25). ESP-IDF's default USB_WRAP
    // route uses FSLS PHY1 (GPIO26/27), so switch it here before installing TinyUSB.
    usb_wrap_ll_phy_select(&USB_WRAP, 0);
#endif
}

static void restore_default_phy_route() {
#if CONFIG_IDF_TARGET_ESP32P4
    usb_wrap_ll_phy_select(&USB_WRAP, 1);
#endif
}

// ---- Controller API ----
// class_config is always a `const tinyusb_config_t*` built by the claiming class's own driver -
// this driver only owns the shared TinyUSB install/uninstall lifecycle and PHY routing, not any
// class's descriptor content.

static error_t claim(struct Device* device, enum UsbDeviceClass usb_class, const void* class_config) {
    auto* ctx = static_cast<UsbDeviceControllerCtx*>(device_get_driver_data(device));

    if (ctx->active_class != USB_DEVICE_CLASS_NONE) {
        LOG_E(TAG, "claim: slot busy (active_class=%d, requested=%d)", ctx->active_class, usb_class);
        return ERROR_RESOURCE_BUSY;
    }
    if (class_config == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }

    route_phy_for_device_mode();
    ctx->phy_routed = true;

    const auto* tusb_cfg = static_cast<const tinyusb_config_t*>(class_config);
    if (tinyusb_driver_install(tusb_cfg) != ESP_OK) {
        LOG_E(TAG, "claim: tinyusb_driver_install failed for class %d", usb_class);
        if (ctx->phy_routed) {
            restore_default_phy_route();
            ctx->phy_routed = false;
        }
        return ERROR_RESOURCE;
    }

    ctx->active_class = usb_class;
    return ERROR_NONE;
}

static error_t release(struct Device* device, enum UsbDeviceClass usb_class) {
    auto* ctx = static_cast<UsbDeviceControllerCtx*>(device_get_driver_data(device));

    if (ctx->active_class != usb_class) {
        LOG_E(TAG, "release: class %d does not hold the slot (active=%d)", usb_class, ctx->active_class);
        return ERROR_INVALID_STATE;
    }

    // Signal disconnect before tearing down so the host notices promptly.
    tud_disconnect();
    vTaskDelay(pdMS_TO_TICKS(250));

    tinyusb_driver_uninstall();

    if (ctx->phy_routed) {
        restore_default_phy_route();
        ctx->phy_routed = false;
    }

    ctx->active_class = USB_DEVICE_CLASS_NONE;
    return ERROR_NONE;
}

static enum UsbDeviceClass get_active_class(struct Device* device) {
    auto* ctx = static_cast<UsbDeviceControllerCtx*>(device_get_driver_data(device));
    return ctx->active_class;
}

extern const UsbDeviceControllerApi usb_device_controller_api = {
    .claim            = claim,
    .release          = release,
    .get_active_class = get_active_class,
};

// ---- Driver lifecycle ----
// This device is defined in each board's .dts (usbdevice0), same pattern as usbhost0 - its
// child classes (usbdevicehid0, usbdevicemsc0, ...) are separate .dts nodes with their own
// drivers, wired to this device as their parent by the devicetree compiler.

extern "C" {

static error_t start_device(struct Device* device) {
    (void)GET_CONFIG(device); // no configuration - placeholder only
    auto* ctx = new UsbDeviceControllerCtx();
    device_set_driver_data(device, ctx);
    return ERROR_NONE;
}

static error_t stop_device(struct Device* device) {
    auto* ctx = static_cast<UsbDeviceControllerCtx*>(device_get_driver_data(device));
    // Safety cleanup: if a class still holds the slot (e.g. this device is stopped while HID/
    // MSC/MIDI is still active), tear TinyUSB down and restore the PHY route before freeing ctx -
    // otherwise the installed TinyUSB driver and mis-routed PHY would outlive the context that
    // tracks them. Mirrors the same safety-release pattern each child driver's own stop_device
    // uses (see esp32_usb_hid_device.cpp / esp32_usb_device_msc.cpp / esp32_usb_midi_device.cpp).
    if (ctx->active_class != USB_DEVICE_CLASS_NONE) {
        release(device, ctx->active_class);
    }
    delete ctx;
    device_set_driver_data(device, nullptr);
    return ERROR_NONE;
}

Driver esp32_usb_device_controller_driver = {
    .name         = "esp32_usb_device_controller",
    .compatible   = (const char*[]) { "espressif,esp32-usbdevice", nullptr },
    .start_device = start_device,
    .stop_device  = stop_device,
    .api          = &usb_device_controller_api,
    .device_type  = &USB_DEVICE_CONTROLLER_TYPE,
    .owner        = nullptr,
    .internal     = nullptr,
};

} // extern "C"

#endif // CONFIG_SOC_USB_OTG_SUPPORTED && (CONFIG_TINYUSB_HID_COUNT || CONFIG_TINYUSB_MSC_ENABLED || CONFIG_TINYUSB_MIDI_COUNT)
