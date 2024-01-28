#include "sdcard.h"
#include "check.h"
#include "log.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define TAG "tdeck_sdcard"
#define SDCARD_SPI_HOST SPI2_HOST

#define BOARD_CS_PIN GPIO_NUM_39
#define RADIO_CS_PIN GPIO_NUM_9
#define BOARD_TFT_CS_PIN GPIO_NUM_12

typedef struct {
    const char* mount_point;
    sdmmc_card_t* card;
} SdcardData;

/**
 * Before we can initialize the sdcard's SPI communications, we have to set all
 * other SPI pins on the board high.
 * See https://github.com/espressif/esp-idf/issues/1597
 * See https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino
 * @return success result
 */
static bool sdcard_set_pins_high() {
    gpio_config_t config = {
        .pin_bit_mask = BIT64(BOARD_CS_PIN) | BIT64(RADIO_CS_PIN) | BIT64(BOARD_TFT_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&config) != ESP_OK) {
        TT_LOG_E(TAG, "GPIO init failed");
        return false;
    }

    if (gpio_set_level(BOARD_CS_PIN, 1) != ESP_OK) {
        TT_LOG_E(TAG, "failed to set board cs high");
        return false;
    }

    if (gpio_set_level(RADIO_CS_PIN, 1) != ESP_OK) {
        TT_LOG_E(TAG, "failed to set radio cs high");
        return false;
    }

    if (gpio_set_level(BOARD_TFT_CS_PIN, 1) != ESP_OK) {
        TT_LOG_E(TAG, "failed to set tft cs high");
        return false;
    }

    return true;
}

static void* sdcard_init(const char* mount_point) {
    if (!sdcard_set_pins_high()) {
        return NULL;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_39;
    slot_config.host_id = SDCARD_SPI_HOST;

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 800000U; // from T-Deck repo's UnitTest.ino project
    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return NULL;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    SdcardData* data = malloc(sizeof(SdcardData));
    *data = (SdcardData) {
        .card = card,
        .mount_point = mount_point
    };

    return data;
}

static void* sdcard_mount(const char* mount_point) {
    TT_LOG_I(TAG, "sdcard init");

    SdcardData* data = sdcard_init(mount_point);
    if (data == NULL) {
        TT_LOG_E(TAG, "sdcard init failed");
        return NULL;
    }

    sdmmc_card_print_info(stdout, data->card);

    return data;
}

static void sdcard_unmount(void* context) {
    SdcardData* data = (SdcardData *)context;
    tt_assert(data != NULL);
    if (esp_vfs_fat_sdcard_unmount(data->mount_point, data->card) != ESP_OK) {
        TT_LOG_E(TAG, "failed to unmount %s", data->mount_point);
    }

    free(data);
}

Sdcard tdeck_sdcard = {
    .mount = &sdcard_mount,
    .unmount = &sdcard_unmount,
    .mount_behaviour = SDCARD_MOUNT_BEHAVIOUR_AT_BOOT
};
