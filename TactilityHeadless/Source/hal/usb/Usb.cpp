#ifdef ESP_PLATFORM

#include <Log.h>
#include "Usb.h"
#include "UsbTusb.h"
#include "TactilityHeadless.h"
#include "hal/SpiSdCard.h"

namespace tt::hal::usb {

#define TAG "usb"

#define BOOT_FLAG 42

struct BootMode {
    uint32_t flag = 0;
};

static Mode currentMode = ModeDefault;
static RTC_NOINIT_ATTR BootMode bootMode;

sdmmc_card_t* _Nullable getCard() {
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
    return isSupported() && (currentMode == ModeDefault || currentMode == ModeNone);
}

bool isSupported() {
    return tusbIsSupported();
}

bool startMassStorageWithSdmmc() {
    if (!canStartNewMode()) {
        TT_LOG_E(TAG, "Can't start");
        return false;
    }

    auto result = tusbStartMassStorageWithSdmmc();
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

    tusbStop();

    currentMode = ModeNone;
}

Mode getMode() {
    return currentMode;
}

bool canRebootIntoMassStorageSdmmc() {
    return tusbIsSupported() && getCard() != nullptr;
}

void rebootIntoMassStorageSdmmc() {
    if (tusbIsSupported()) {
        bootMode.flag = BOOT_FLAG;
        esp_restart();
    }
}

bool isUsbBootMode() {
    return bootMode.flag == BOOT_FLAG;
}

void resetUsbBootMode() {
    bootMode.flag = 0;
}

}

#endif
