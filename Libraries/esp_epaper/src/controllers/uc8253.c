/**
 * @file uc8253.c
 * @brief Driver for UC8253 based B/W e-paper displays
 *
 * Supports panels: GDEQ031T10 (3.1" 240x320)
 * Follows the GoodDisplay reference sequence: every update resets and
 * re-initializes the controller, and powers it off afterwards.
 */

#include "../epaper_common.h"
#include "epaper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "uc8253";

static void uc8253_power_on(epd_device_t *dev)
{
    epd_spi_t *spi = epd_get_spi(dev);

    RESET(spi);

    CMD(spi, 0x00);  // Panel setting: KW mode, LUT from OTP
    DATA(spi, 0x1F);

    CMD(spi, 0x04);  // Power on
    WAIT(dev);
}

static void uc8253_power_off(epd_device_t *dev)
{
    epd_spi_t *spi = epd_get_spi(dev);

    CMD(spi, 0x02);  // Power off
    WAIT(dev);
}

/*=============================================================================
 * Initialization
 *============================================================================*/

esp_err_t uc8253_init(epd_device_t *dev)
{
    uc8253_power_on(dev);
    uc8253_power_off(dev);

    ESP_LOGI(TAG, "Panel initialized: %dx%d", epd_get_width(dev), epd_get_height(dev));
    return ESP_OK;
}

/*=============================================================================
 * Update Functions
 *============================================================================*/

esp_err_t uc8253_update(epd_device_t *dev, epd_update_mode_t mode)
{
    epd_spi_t *spi = epd_get_spi(dev);
    const uint8_t *frame = epd_get_framebuffer(dev);
    const uint32_t len = (uint32_t)(epd_get_width(dev) / 8) * epd_get_height(dev);

    uc8253_power_on(dev);

    if (mode == EPD_UPDATE_FAST) {
        CMD(spi, 0xE0);  // Cascade setting: use forced temperature
        DATA(spi, 0x02);
        CMD(spi, 0xE5);  // Force temperature: selects the fast (~1s) OTP waveform
        DATA(spi, 0x5A);
    }

    CMD(spi, 0x10);  // Old data
    epd_spi_write_data_bulk(spi, frame, len);
    CMD(spi, 0x13);  // New data
    epd_spi_write_data_bulk(spi, frame, len);

    CMD(spi, 0x12);  // Display refresh
    vTaskDelay(pdMS_TO_TICKS(1));  // >= 200us before BUSY is valid
    WAIT(dev);

    uc8253_power_off(dev);
    return ESP_OK;
}

/*=============================================================================
 * RAM Write Functions
 *============================================================================*/

esp_err_t uc8253_write_ram(epd_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    (void)data;
    (void)len;
    // The reset at the start of every update clears RAM, so uc8253_update() sends the framebuffer
    return ESP_OK;
}

/*=============================================================================
 * Power Management
 *============================================================================*/

esp_err_t uc8253_sleep(epd_device_t *dev)
{
    epd_spi_t *spi = epd_get_spi(dev);

    uc8253_power_off(dev);
    CMD(spi, 0x07);  // Deep sleep
    DATA(spi, 0xA5);

    ESP_LOGI(TAG, "Panel in deep sleep");
    return ESP_OK;
}

esp_err_t uc8253_wake(epd_device_t *dev)
{
    return uc8253_init(dev);
}
