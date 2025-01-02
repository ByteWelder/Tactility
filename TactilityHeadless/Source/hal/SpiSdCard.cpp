#ifdef ESP_PLATFORM

#include "SpiSdCard.h"

#include "Check.h"
#include "Log.h"

#include <driver/gpio.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define TAG "spi_sdcard"

namespace tt::hal {

/**
 * Before we can initialize the sdcard's SPI communications, we have to set all
 * other SPI pins on the board high.
 * See https://github.com/espressif/esp-idf/issues/1597
 * See https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino
 * @return success result
 */
bool SpiSdCard::applyGpioWorkAround() {
    TT_LOG_D(TAG, "init");

    uint64_t pin_bit_mask = BIT64(config->spiPinCs);
    for (auto const& pin: config->csPinWorkAround) {
        pin_bit_mask |= BIT64(pin);
    }

    gpio_config_t sd_gpio_config = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&sd_gpio_config) != ESP_OK) {
        TT_LOG_E(TAG, "GPIO init failed");
        return false;
    }

    for (auto const& pin: config->csPinWorkAround) {
        if (gpio_set_level(pin, 1) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to set board CS pin high");
            return false;
        }
    }

    return true;
}

bool SpiSdCard::mountInternal(const char* mountPoint) {
    TT_LOG_I(TAG, "Mounting %s", mountPoint);

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
    host.max_freq_khz = config->spiFrequency;
    host.slot = config->spiHost;

    esp_err_t result = esp_vfs_fat_sdspi_mount(mountPoint, &host, &slot_config, &mount_config, &card);

    if (result != ESP_OK) {
        if (result == ESP_FAIL) {
            TT_LOG_E(TAG, "Mounting failed. Ensure the card is formatted with FAT.");
        } else {
            TT_LOG_E(TAG, "Mounting failed (%s)", esp_err_to_name(result));
        }
        return false;
    }

    this->mountPoint = mountPoint;

    return true;
}

bool SpiSdCard::mount(const char* mount_point) {
    if (!applyGpioWorkAround()) {
        TT_LOG_E(TAG, "Failed to set SPI CS pins high. This is a pre-requisite for mounting.");
        return false;
    }

    if (mountInternal(mount_point)) {
        sdmmc_card_print_info(stdout, card);
        return true;
    } else {
        TT_LOG_E(TAG, "Mount failed for %s", mount_point);
        return false;
    }
}

bool SpiSdCard::unmount() {
    if (card == nullptr) {
        TT_LOG_E(TAG, "Can't unmount: not mounted");
        return false;
    }

    if (esp_vfs_fat_sdcard_unmount(mountPoint.c_str(), card) == ESP_OK) {
        mountPoint = "";
        card = nullptr;
        return true;
    } else {
        TT_LOG_E(TAG, "Unmount failed for %s", mountPoint.c_str());
        return false;
    }
}

// TODO: Refactor to "bool getStatus(Status* status)" method so that it can fail when the lvgl lock fails
tt::hal::SdCard::State SpiSdCard::getState() const {
    if (card == nullptr) {
        return StateUnmounted;
    }

    /**
     * The SD card and the screen are on the same SPI bus.
     * Writing and reading to the bus from 2 devices at the same time causes crashes.
     * This work-around ensures that this check is only happening when LVGL isn't rendering.
     */
    if (config->lockable) {
        bool locked = config->lockable->lock(50); // TODO: Refactor to a more reliable locking mechanism
        if (!locked) {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
            return StateUnknown;
        }
    }

    bool result = sdmmc_get_status(card) == ESP_OK;

    if (config->lockable) {
        config->lockable->unlock();
    }

    if (result) {
        return StateMounted;
    } else {
        return StateError;
    }
}

}

#endif