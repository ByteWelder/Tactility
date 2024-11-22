#include "check.h"
#include "log.h"
#include "hal/Sdcard.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define TAG "m5stack_core2_sdcard"

#define CORE2_SDCARD_SPI_HOST SPI2_HOST
#define CORE2_SDCARD_PIN_CS GPIO_NUM_4
#define CORE2_SDCARD_SPI_FREQUENCY 800000U
#define CORE2_SDCARD_FORMAT_ON_MOUNT_FAILED false
#define CORE2_SDCARD_MAX_OPEN_FILES 4
#define CORE2_SDCARD_ALLOC_UNIT_SIZE (16 * 1024)
#define CORE2_SDCARD_STATUS_CHECK_ENABLED false

typedef struct {
    const char* mount_point;
    sdmmc_card_t* card;
} MountData;

static void* sdcard_mount(const char* mount_point) {
    TT_LOG_I(TAG, "Mounting %s", mount_point);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = CORE2_SDCARD_FORMAT_ON_MOUNT_FAILED,
        .max_files = CORE2_SDCARD_MAX_OPEN_FILES,
        .allocation_unit_size = CORE2_SDCARD_ALLOC_UNIT_SIZE,
        .disk_status_check_enable = CORE2_SDCARD_STATUS_CHECK_ENABLED
    };

    sdmmc_card_t* card;

    // Init without card detect (CD) and write protect (WD)
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CORE2_SDCARD_PIN_CS;
    slot_config.host_id = CORE2_SDCARD_SPI_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = CORE2_SDCARD_SPI_FREQUENCY;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            TT_LOG_E(TAG, "Mounting failed. Ensure the card is formatted with FAT.");
        } else {
            TT_LOG_E(TAG, "Mounting failed (%s)", esp_err_to_name(ret));
        }
        return nullptr;
    }

    auto* data = static_cast<MountData*>(malloc(sizeof(MountData)));
    *data = (MountData) {
        .mount_point = mount_point,
        .card = card,
    };

    sdmmc_card_print_info(stdout, data->card);

    return data;
}

static void sdcard_unmount(void* context) {
    auto* data = static_cast<MountData*>(context);
    TT_LOG_I(TAG, "Unmounting %s", data->mount_point);

    tt_assert(data != nullptr);
    if (esp_vfs_fat_sdcard_unmount(data->mount_point, data->card) != ESP_OK) {
        TT_LOG_E(TAG, "Unmount failed for %s", data->mount_point);
    }

    free(data);
}

static bool sdcard_is_mounted(void* context) {
    auto* data = static_cast<MountData*>(context);
    return (data != nullptr) && (sdmmc_get_status(data->card) == ESP_OK);
}

extern const tt::hal::sdcard::SdCard m5stack_core2_sdcard = {
    .mount = &sdcard_mount,
    .unmount = &sdcard_unmount,
    .is_mounted = &sdcard_is_mounted,
    .mount_behaviour = tt::hal::sdcard::MountBehaviourAnytime
};
