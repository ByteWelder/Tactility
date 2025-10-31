#ifdef ESP_PLATFORM

#include <Tactility/hal/usb/Usb.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/hal/usb/UsbTusb.h>

#include <Tactility/Log.h>

namespace tt::hal::usb {

constexpr auto* TAG = "usb";
constexpr auto BOOT_FLAG_SDMMC = 42;  // Existing
constexpr auto BOOT_FLAG_FLASH = 43;  //  For flash mode

struct BootMode {
    uint32_t flag = 0;
};

static Mode currentMode = Mode::Default;
static RTC_NOINIT_ATTR BootMode bootMode;

sdmmc_card_t* _Nullable getCard() {
    auto sdcards = findDevices<sdcard::SpiSdCardDevice>(Device::Type::SdCard);

    std::shared_ptr<sdcard::SpiSdCardDevice> usable_sdcard;
    for (auto& sdcard : sdcards) {
        auto sdcard_candidate = std::static_pointer_cast<sdcard::SpiSdCardDevice>(sdcard);
        if (sdcard_candidate != nullptr && sdcard_candidate->isMounted() && sdcard_candidate->getCard() != nullptr) {
            usable_sdcard = sdcard_candidate;
            break;
        }
    }

    if (usable_sdcard == nullptr) {
        TT_LOG_W(TAG, "Couldn't find a mounted SpiSdCard");
        return nullptr;
    }

    auto* sdmmc_card = usable_sdcard->getCard();
    if (sdmmc_card == nullptr) {
        TT_LOG_W(TAG, "SD card has no card object available");
        return nullptr;
    }

    return sdmmc_card;
}

static bool canStartNewMode() {
    return isSupported() && (currentMode == Mode::Default || currentMode == Mode::None);
}

bool isSupported() {
    return tusbIsSupported();
}

bool startMassStorageWithSdmmc() {
    if (!canStartNewMode()) {
        TT_LOG_E(TAG, "Can't start");
        return false;
    }

    if (tusbStartMassStorageWithSdmmc()) {
        currentMode = Mode::MassStorageSdmmc;
        return true;
    } else {
        TT_LOG_E(TAG, "Failed to init mass storage");
        return false;
    }
}

void stop() {
    if (canStartNewMode()) {
        return;
    }

    tusbStop();

    currentMode = Mode::None;
}

Mode getMode() {
    return currentMode;
}

bool canRebootIntoMassStorageSdmmc() {
    return tusbIsSupported() && getCard() != nullptr;
}

void rebootIntoMassStorageSdmmc() {
    if (tusbIsSupported()) {
        bootMode.flag = BOOT_FLAG_SDMMC;
        esp_restart();
    }
}

bool isUsbBootMode() {
    return bootMode.flag == BOOT_FLAG_SDMMC || bootMode.flag == BOOT_FLAG_FLASH;  // Support both
}

void resetUsbBootMode() {
    bootMode.flag = 0;
}

// Flash mass storage functions
bool startMassStorageWithFlash() {
    if (!canStartNewMode()) {
        TT_LOG_E(TAG, "Can't start flash mass storage");
        return false;
    }

    if (tusbStartMassStorageWithFlash()) {
        currentMode = Mode::MassStorageFlash;
        return true;
    } else {
        TT_LOG_E(TAG, "Failed to init flash mass storage");
        return false;
    }
}

bool canRebootIntoMassStorageFlash() {
    return tusbIsSupported();  // No SD card needed, just TinyUSB support
}

void rebootIntoMassStorageFlash() {
    if (tusbIsSupported()) {
        bootMode.flag = BOOT_FLAG_FLASH;
        esp_restart();
    }
}

}

#endif
