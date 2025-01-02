#ifdef ESP_PLATFORM

#include <Log.h>
#include "Usb.h"

#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include "TactilityHeadless.h"
#include "hal/SpiSdCard.h"

namespace tt::hal::usb {

#define TAG "usb"

#define BOOT_FLAG 42

struct BootMode {
    uint32_t flag = 0;
};

static Mode currentMode = ModeDefault;
static bool driverInstalled = false;
static RTC_NOINIT_ATTR BootMode bootMode;

#define EPNUM_MSC       1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

enum {
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};

enum {
    EDPT_CTRL_OUT = 0x00,
    EDPT_CTRL_IN  = 0x80,

    EDPT_MSC_OUT  = 0x01,
    EDPT_MSC_IN   = 0x81,
};

static tusb_desc_device_t descriptor_config = {
    .bLength = sizeof(descriptor_config),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
    .idProduct = 0x4002,
    .bcdDevice = 0x100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "Espressif",                    // 1: Manufacturer
    "Tactility Device",             // 2: Product
    "42",                           // 3: Serials
    "Tactility Mass Storage",       // 4. MSC
};


static uint8_t const msc_fs_configuration_desc[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};

#if (TUD_OPT_HIGH_SPEED)
    static const tusb_desc_device_qualifier_t device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0
};

static uint8_t const msc_hs_configuration_desc[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 512),
};
#endif // TUD_OPT_HIGH_SPEED

static sdmmc_card_t* _Nullable getCard() {
    auto sdcard = getConfiguration().sdcard;
    if (sdcard == nullptr) {
        TT_LOG_W(TAG, "No SD card configuration found");
        return nullptr;
    }

    if (!sdcard->isMounted()) {
        TT_LOG_W(TAG, "SD card not mounted");
        return nullptr;
    }

    auto spi_sdcard = std::static_pointer_cast<SpiSdCard>(sdcard);
    if (spi_sdcard == nullptr) {
        TT_LOG_W(TAG, "SD card interface is not supported (must be SpiSdCard)");
        return nullptr;
    }

    auto* card = spi_sdcard->getCard();
    if (card == nullptr) {
        TT_LOG_W(TAG, "SD card has no card object available");
        return nullptr;
    }

    return card;
}

static bool canStartNewMode() {
    return currentMode == ModeDefault || currentMode == ModeNone;
}

static void storage_mount_changed_cb(tinyusb_msc_event_t* event) {
    if (event->mount_changed_data.is_mounted) {
        TT_LOG_I(TAG, "Mounted");
    } else {
        TT_LOG_I(TAG, "Unmounted");
    }
}

static bool ensureDriverInstalled() {
    if (driverInstalled) {
        return true;
    }

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor_config,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = msc_fs_configuration_desc,
        .hs_configuration_descriptor = msc_hs_configuration_desc,
        .qualifier_descriptor = &device_qualifier,
#else
        .configuration_descriptor = msc_fs_configuration_desc,
#endif // TUD_OPT_HIGH_SPEED
        .self_powered = false,
        .vbus_monitor_io = 0
    };

    if (tinyusb_driver_install(&tusb_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to install TinyUSB driver");
        return false;
    }

    driverInstalled = true;
    return true;
}

bool startMassStorageWithSdmmc() {
    if (!canStartNewMode()) {
        TT_LOG_E(TAG, "Already started");
        return false;
    }

    ensureDriverInstalled();

    auto* card = getCard();
    if (card == nullptr) {
        TT_LOG_E(TAG, "SD card not mounted");
        return false;
    }

    const tinyusb_msc_sdmmc_config_t config_sdmmc = {
        .card = card,
        .callback_mount_changed = storage_mount_changed_cb,
        .callback_premount_changed = nullptr,
        .mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 0,
            .disk_status_check_enable = false,
            .use_one_fat = false
        }
    };

    auto result = tinyusb_msc_storage_init_sdmmc(&config_sdmmc);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init mass storage: %s", esp_err_to_name(result));
        return false;
    } else {
        currentMode = ModeMassStorageSdmmc;
        return true;
    }
}

void stop() {
    if (canStartNewMode()) {
        return;
    }

    tinyusb_msc_storage_deinit();
    currentMode = ModeNone;
}

Mode getMode() {
    return currentMode;
}

bool canRebootIntoMassStorageSdmmc() {
    return getCard() != nullptr;
}

void rebootIntoMassStorageSdmmc() {
    bootMode.flag = BOOT_FLAG;
    esp_restart();
}

bool isUsbBootMode() {
    return bootMode.flag == BOOT_FLAG;
}

void resetUsbBootMode() {
    bootMode.flag = 0;
}

}

#endif
