#ifdef ESP_PLATFORM

#include <Tactility/hal/gpio/Gpio.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/Log.h>

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

namespace tt::hal::sdcard {

constexpr auto* TAG = "SpiSdCardDevice";

/**
 * Before we can initialize the sdcard's SPI communications, we have to set all
 * other SPI pins on the board high.
 * See https://github.com/espressif/esp-idf/issues/1597
 * See https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino
 * @return success result
 */
bool SpiSdCardDevice::applyGpioWorkAround() {
    TT_LOG_D(TAG, "init");

    uint64_t pin_bit_mask = BIT64(config->spiPinCs);
    for (auto const& pin: config->csPinWorkAround) {
        pin_bit_mask |= BIT64(pin);
    }

    if (!gpio::configureWithPinBitmask(pin_bit_mask, gpio::Mode::Output, false, false)) {
        TT_LOG_E(TAG, "GPIO init failed");
        return false;
    }

    for (auto const& pin: config->csPinWorkAround) {
        if (!gpio::setLevel(pin, true)) {
            TT_LOG_E(TAG, "Failed to set board CS pin high");
            return false;
        }
    }

    return true;
}

bool SpiSdCardDevice::mountInternal(const std::string& newMountPath) {
    TT_LOG_I(TAG, "Mounting %s", newMountPath.c_str());

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = config->formatOnMountFailed,
        .max_files = config->maxOpenFiles,
        .allocation_unit_size = config->allocUnitSize,
        .disk_status_check_enable = config->statusCheckEnabled,
        .use_one_fat = false
    };

    // Init without card detect (CD) and write protect (WD)
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = config->spiHost;
    slot_config.gpio_cs = config->spiPinCs;
    slot_config.gpio_cd = config->spiPinCd;
    slot_config.gpio_wp = config->spiPinWp;
    slot_config.gpio_int = config->spiPinInt;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    // The following value is from T-Deck repo's UnitTest.ino project:
    // https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino
    // Observation: Using this automatically sets the bus to 20MHz
    host.max_freq_khz = config->spiFrequencyKhz;
    host.slot = config->spiHost;

    esp_err_t result = esp_vfs_fat_sdspi_mount(newMountPath.c_str(), &host, &slot_config, &mount_config, &card);

    if (result != ESP_OK || card == nullptr) {
        if (result == ESP_FAIL) {
            TT_LOG_E(TAG, "Mounting failed. Ensure the card is formatted with FAT.");
        } else {
            TT_LOG_E(TAG, "Mounting failed (%s)", esp_err_to_name(result));
        }
        return false;
    }

    mountPath = newMountPath;

    return true;
}

bool SpiSdCardDevice::mount(const std::string& newMountPath) {
    if (!applyGpioWorkAround()) {
        TT_LOG_E(TAG, "Failed to apply GPIO work-around");
        return false;
    }

    if (mountInternal(newMountPath)) {
        TT_LOG_I(TAG, "Mounted at %s", newMountPath.c_str());
        sdmmc_card_print_info(stdout, card);
        return true;
    } else {
        TT_LOG_E(TAG, "Mount failed for %s", newMountPath.c_str());
        return false;
    }
}

bool SpiSdCardDevice::unmount() {
    if (card == nullptr) {
        TT_LOG_E(TAG, "Can't unmount: not mounted");
        return false;
    }

    if (esp_vfs_fat_sdcard_unmount(mountPath.c_str(), card) != ESP_OK) {
        TT_LOG_E(TAG, "Unmount failed for %s", mountPath.c_str());
        return false;
    }

    TT_LOG_I(TAG, "Unmounted %s", mountPath.c_str());
    mountPath = "";
    card = nullptr;
    return true;
}

// TODO: Refactor to "bool getStatus(Status* status)" method so that it can fail when the lvgl lock fails
SdCardDevice::State SpiSdCardDevice::getState(TickType_t timeout) const {
    if (card == nullptr) {
        return State::Unmounted;
    }

    /**
     * The SD card and the screen are on the same SPI bus.
     * Writing and reading to the bus from 2 devices at the same time causes crashes.
     * This work-around ensures that this check is only happening when LVGL isn't rendering.
     */
    auto lock = getLock()->asScopedLock();
    bool locked = lock.lock(timeout);
    if (!locked) {
        return State::Timeout;
    }

    if (sdmmc_get_status(card) != ESP_OK) {
        return State::Error;
    }

    return State::Mounted;
}

}

#endif