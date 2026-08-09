#include <sdkconfig.h>
#if CONFIG_SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MSC_ENABLED

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/esp32_usbdevice.h>
#include <tactility/drivers/usb_device_controller.h>
#include <tactility/drivers/usb_msc_device.h>
#include <tactility/log.h>

#include <tinyusb.h>
#include <tusb_msc_storage.h>
#include <wear_levelling.h>

#define TAG "esp32_usb_device_msc"
#define GET_CONFIG(device) ((const Esp32UsbDeviceChildConfig*)(device)->config)
#define EPNUM_MSC 1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

// ---- MSC device descriptor set ----

enum {
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};

enum {
    EDPT_MSC_OUT = 0x01,
    EDPT_MSC_IN  = 0x81,
};

static const tusb_desc_device_t msc_device_descriptor = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4002,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

#if (TUD_OPT_HIGH_SPEED)
static const tusb_desc_device_qualifier_t msc_device_qualifier = {
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

static const char* msc_string_descriptor[] = {
    (const char[]) { 0x09, 0x04 },  // 0: English (0x0409)
    "Tactility",                    // 1: Manufacturer
    "Tactility Device",             // 2: Product
    "42",                           // 3: Serial
    "Tactility Mass Storage",       // 4: MSC
};

static const uint8_t msc_fs_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};

#if (TUD_OPT_HIGH_SPEED)
static const uint8_t msc_hs_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 512),
};
#endif

static const tinyusb_config_t msc_tusb_cfg = {
    .device_descriptor = &msc_device_descriptor,
    .string_descriptor = msc_string_descriptor,
    .string_descriptor_count = sizeof(msc_string_descriptor) / sizeof(msc_string_descriptor[0]),
    .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = msc_fs_configuration_descriptor,
    .hs_configuration_descriptor = msc_hs_configuration_descriptor,
    .qualifier_descriptor = &msc_device_qualifier,
#else
    .configuration_descriptor = msc_fs_configuration_descriptor,
#endif
    .self_powered = false,
    .vbus_monitor_io = 0,
};

// ---- MSC device state ----

struct UsbMscDeviceCtx {
    UsbMscDeviceMountChangedCallback mount_changed_cb = nullptr;
    void* mount_changed_context = nullptr;
    bool storage_active = false;
};

// device pointer isn't threaded through the TinyUSB mount-changed callback, so this driver
// supports a single active MSC device instance at a time - matches the single TinyUSB
// device-mode slot the controller already enforces.
static struct Device* active_msc_device = nullptr;

static void storage_mount_changed_cb(tinyusb_msc_event_t* event) {
    if (active_msc_device == nullptr) return;
    auto* ctx = static_cast<UsbMscDeviceCtx*>(device_get_driver_data(active_msc_device));
    if (ctx == nullptr) return;

    const bool mounted = event->mount_changed_data.is_mounted;
    LOG_I(TAG, "%s", mounted ? "MSC mounted" : "MSC unmounted");
    if (ctx->mount_changed_cb != nullptr) {
        ctx->mount_changed_cb(mounted, ctx->mount_changed_context);
    }
}

// ---- MSC device API ----

static error_t msc_device_start(struct Device* device, enum UsbMscDeviceSource source, void* source_handle,
                                UsbMscDeviceMountChangedCallback mount_changed_cb, void* context) {
    if (source_handle == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* controller = device_get_parent(device);
    error_t claim_result = usb_device_controller_claim(controller, USB_DEVICE_CLASS_MSC, &msc_tusb_cfg);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    auto* ctx = static_cast<UsbMscDeviceCtx*>(device_get_driver_data(device));
    ctx->mount_changed_cb = mount_changed_cb;
    ctx->mount_changed_context = context;
    active_msc_device = device;

    esp_err_t result;
    if (source == USB_MSC_DEVICE_SOURCE_SDMMC) {
        const tinyusb_msc_sdmmc_config_t config_sdmmc = {
            .card = static_cast<sdmmc_card_t*>(source_handle),
            .callback_mount_changed = storage_mount_changed_cb,
            .callback_premount_changed = nullptr,
            .mount_config = {
                .format_if_mount_failed = false,
                .max_files = 5,
                .allocation_unit_size = 0,
                .disk_status_check_enable = false,
                .use_one_fat = false,
            },
        };
        result = tinyusb_msc_storage_init_sdmmc(&config_sdmmc);
    } else {
        const tinyusb_msc_spiflash_config_t config_flash = {
            .wl_handle = reinterpret_cast<wl_handle_t>(source_handle),
            .callback_mount_changed = storage_mount_changed_cb,
            .callback_premount_changed = nullptr,
            .mount_config = {
                .format_if_mount_failed = false,
                .max_files = 5,
                .allocation_unit_size = 0,
                .disk_status_check_enable = false,
                .use_one_fat = false,
            },
        };
        result = tinyusb_msc_storage_init_spiflash(&config_flash);
    }

    if (result != ESP_OK) {
        LOG_E(TAG, "storage init failed: %s", esp_err_to_name(result));
        active_msc_device = nullptr;
        usb_device_controller_release(controller, USB_DEVICE_CLASS_MSC);
        return ERROR_RESOURCE;
    }

    ctx->storage_active = true;
    return ERROR_NONE;
}

static error_t msc_device_stop(struct Device* device) {
    auto* ctx = static_cast<UsbMscDeviceCtx*>(device_get_driver_data(device));
    if (ctx == nullptr || !ctx->storage_active) {
        return ERROR_NONE;
    }

    tinyusb_msc_storage_deinit();

    auto* controller = device_get_parent(device);
    usb_device_controller_release(controller, USB_DEVICE_CLASS_MSC);

    ctx->storage_active = false;
    ctx->mount_changed_cb = nullptr;
    ctx->mount_changed_context = nullptr;
    if (active_msc_device == device) {
        active_msc_device = nullptr;
    }
    return ERROR_NONE;
}

static bool msc_device_is_connected(struct Device* device) {
    auto* ctx = static_cast<UsbMscDeviceCtx*>(device_get_driver_data(device));
    return ctx != nullptr && ctx->storage_active && tud_mounted();
}

extern const UsbMscDeviceApi esp32_usb_msc_device_api = {
    .start        = msc_device_start,
    .stop         = msc_device_stop,
    .is_connected = msc_device_is_connected,
};

// ---- Driver lifecycle ----
// Defined in each board's .dts as a child of usbdevice0 (e.g. usbdevicemsc0).

extern "C" {

static error_t start_device(struct Device* device) {
    (void)GET_CONFIG(device); // no configuration - placeholder only
    auto* ctx = new UsbMscDeviceCtx();
    device_set_driver_data(device, ctx);
    return ERROR_NONE;
}

static error_t stop_device(struct Device* device) {
    auto* ctx = static_cast<UsbMscDeviceCtx*>(device_get_driver_data(device));
    if (ctx != nullptr && ctx->storage_active) {
        msc_device_stop(device);
    }
    delete ctx;
    device_set_driver_data(device, nullptr);
    return ERROR_NONE;
}

Driver esp32_usb_msc_device_driver = {
    .name         = "esp32_usb_msc_device",
    .compatible   = (const char*[]) { "espressif,esp32-usbdevice-msc", nullptr },
    .start_device = start_device,
    .stop_device  = stop_device,
    .api          = &esp32_usb_msc_device_api,
    .device_type  = &USB_MSC_DEVICE_TYPE,
    .owner        = nullptr,
    .internal     = nullptr,
};

} // extern "C"

#endif // CONFIG_SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MSC_ENABLED
