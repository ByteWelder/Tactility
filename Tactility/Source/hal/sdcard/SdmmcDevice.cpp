#ifdef ESP_PLATFORM

#include <Tactility/hal/sdcard/SdmmcDevice.h>
#include <Tactility/Log.h>

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>

namespace tt::hal::sdcard {

constexpr auto* TAG = "SdmmcDevice";

bool SdmmcDevice::mountInternal(const std::string& newMountPath) {
    TT_LOG_I(TAG, "Mounting %s", newMountPath.c_str());

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = config->formatOnMountFailed,
        .max_files = config->maxOpenFiles,
        .allocation_unit_size = config->allocUnitSize,
        .disk_status_check_enable = config->statusCheckEnabled,
        .use_one_fat = false
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = {
        .clk = config->pinClock,
        .cmd = config->pinCmd,
        .d0  = config->pinD0,
        .d1  = config->pinD1,
        .d2  = config->pinD2,
        .d3  = config->pinD3,
        .d4  = static_cast<gpio_num_t>(0),
        .d5  = static_cast<gpio_num_t>(0),
        .d6  = static_cast<gpio_num_t>(0),
        .d7  = static_cast<gpio_num_t>(0),
        .cd  = GPIO_NUM_NC,
        .wp  = GPIO_NUM_NC,
        .width = 4,
        .flags = 0
      };

    esp_err_t result = esp_vfs_fat_sdmmc_mount(newMountPath.c_str(), &host, &slot_config, &mount_config, &card);

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

bool SdmmcDevice::mount(const std::string& newMountPath) {
    auto lock = getLock()->asScopedLock();
    lock.lock();

    if (mountInternal(newMountPath)) {
        TT_LOG_I(TAG, "Mounted at %s", newMountPath.c_str());
        sdmmc_card_print_info(stdout, card);
        return true;
    } else {
        TT_LOG_E(TAG, "Mount failed for %s", newMountPath.c_str());
        return false;
    }
}

bool SdmmcDevice::unmount() {
    auto lock = getLock()->asScopedLock();
    lock.lock();

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

SdmmcDevice::State SdmmcDevice::getState(TickType_t timeout) const {
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