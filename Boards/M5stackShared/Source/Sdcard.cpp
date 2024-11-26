#include "Check.h"
#include "Log.h"
#include "hal/sdcard/Sdcard.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define TAG "m5stack_sdcard"

#define SDCARD_SPI_HOST SPI2_HOST
#define SDCARD_PIN_CS GPIO_NUM_4
#define SDCARD_SPI_FREQUENCY 800000U
#define SDCARD_FORMAT_ON_MOUNT_FAILED false
#define SDCARD_MAX_OPEN_FILES 4
#define SDCARD_ALLOC_UNIT_SIZE (16 * 1024)
#define SDCARD_STATUS_CHECK_ENABLED false

typedef struct {
    const char* mount_point;
    sdmmc_card_t* card;
} MountData;


/**
 * Before we can initialize the sdcard's SPI communications, we have to set all
 * other SPI pins on the board high.
 * See https://github.com/espressif/esp-idf/issues/1597
 * See https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino
 * @return success result
 */
static bool sdcard_init() {
    TT_LOG_D(TAG, "init");

    gpio_config_t config = {
        .pin_bit_mask = BIT64(GPIO_NUM_4) | BIT64(GPIO_NUM_5),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&config) != ESP_OK) {
        TT_LOG_E(TAG, "GPIO init failed");
        return false;
    }

    if (gpio_set_level(GPIO_NUM_4, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set board CS pin high");
        return false;
    }

    if (gpio_set_level(GPIO_NUM_5, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set board CS pin high");
        return false;
    }

    return true;
}

static void* sdcard_mount(const char* mount_point) {
    TT_LOG_I(TAG, "Mounting %s", mount_point);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = SDCARD_FORMAT_ON_MOUNT_FAILED,
        .max_files = SDCARD_MAX_OPEN_FILES,
        .allocation_unit_size = SDCARD_ALLOC_UNIT_SIZE,
        .disk_status_check_enable = SDCARD_STATUS_CHECK_ENABLED,
        .use_one_fat = false
    };

    sdmmc_card_t* card;

    // Init without card detect (CD) and write protect (WD)
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_PIN_CS;
    slot_config.host_id = SDCARD_SPI_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDCARD_SPI_FREQUENCY;
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

    return data;
}


static void* sdcard_init_and_mount(const char* mount_point) {
    if (!sdcard_init()) {
        TT_LOG_E(TAG, "Failed to set SPI CS pins high. This is a pre-requisite for mounting.");
        return NULL;
    }
    auto* data = static_cast<MountData*>(sdcard_mount(mount_point));
    if (data == nullptr) {
        TT_LOG_E(TAG, "Mount failed for %s", mount_point);
        return nullptr;
    }

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

extern const tt::hal::sdcard::SdCard m5stack_sdcard = {
    .mount = &sdcard_init_and_mount,
    .unmount = &sdcard_unmount,
    .is_mounted = &sdcard_is_mounted,
    .mount_behaviour = tt::hal::sdcard::MountBehaviourAnytime
};
