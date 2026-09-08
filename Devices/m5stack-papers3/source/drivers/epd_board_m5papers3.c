// SPDX-License-Identifier: Apache-2.0
/**
 * Board definition for M5Stack PaperS3.
 *
 * Kept out-of-tree (see epd_board_m5papers3.h) instead of forking epdiy: this file only
 * uses epdiy's public API, so it builds as an ordinary consumer of the upstream component.
 *
 * Pin mapping from M5GFX source code (authoritative reference):
 * https://github.com/m5stack/M5GFX/blob/master/src/M5GFX.cpp
 *
 * Data bus: DB0-DB7 on GPIO 6,14,7,12,9,11,8,10
 * Control: STH=13, LEH=15, STV=17, CKV=18, CKH=16
 * Power: PWR=46, OE=45
 */

#include "epd_board_m5papers3.h"

#include <stdint.h>
#include "epdiy.h"

#include <output_lcd/lcd_driver.h>
#include "esp_log.h"

#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef CONFIG_IDF_TARGET_ESP32S3
#error "M5Paper S3 board only supports ESP32-S3"
#endif

static const char* TAG = "m5paper_s3";

/* Data Lines - from M5GFX source */
#define D0 GPIO_NUM_6
#define D1 GPIO_NUM_14
#define D2 GPIO_NUM_7
#define D3 GPIO_NUM_12
#define D4 GPIO_NUM_9
#define D5 GPIO_NUM_11
#define D6 GPIO_NUM_8
#define D7 GPIO_NUM_10

/* Control Lines - from M5GFX source */
#define STH GPIO_NUM_13   /* Start pulse horizontal (active low) */
#define LEH GPIO_NUM_15   /* Latch enable horizontal */
#define STV GPIO_NUM_17   /* Start vertical */
#define CKV GPIO_NUM_18   /* Clock vertical */
#define CKH GPIO_NUM_16   /* Clock horizontal - LCD peripheral clock output */

/* Power control - from M5GFX source */
#define PWR_PIN GPIO_NUM_46   /* Main power enable */
#define OE_PIN  GPIO_NUM_45   /* Output enable */


static lcd_bus_config_t lcd_config = {
    .clock = CKH,
    .ckv = CKV,
    .leh = LEH,
    .start_pulse = STH,
    .stv = STV,
    .data[0] = D0,
    .data[1] = D1,
    .data[2] = D2,
    .data[3] = D3,
    .data[4] = D4,
    .data[5] = D5,
    .data[6] = D6,
    .data[7] = D7,
};

static void epd_board_init(uint32_t epd_row_width, const EpdInitConfig* init_config) {
    (void)init_config;
    ESP_LOGI(TAG, "Initializing M5Paper S3 board");

    /* Configure power pin - start with power off */
    gpio_reset_pin(PWR_PIN);
    gpio_set_direction(PWR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PWR_PIN, 0);

    /* Configure output enable pin - active high */
    gpio_reset_pin(OE_PIN);
    gpio_set_direction(OE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OE_PIN, 0);

    const EpdDisplay_t* display = epd_get_display();

    LcdEpdConfig_t config = {
        .pixel_clock = display->bus_speed * 1000 * 1000,
        .ckv_high_time = 60,
        .line_front_porch = 4,
        .le_high_time = 4,
        .bus_width = display->bus_width,
        .bus = lcd_config,
    };

    epd_lcd_init(&config, display->width, display->height);

    ESP_LOGI(TAG, "Board initialized: %dx%d @ %dMHz",
             display->width, display->height, display->bus_speed);
}

static void epd_board_deinit() {
    ESP_LOGI(TAG, "Deinitializing M5Paper S3 board");

    /* Disable output first */
    gpio_set_level(OE_PIN, 0);

    /* Power off display */
    gpio_set_level(PWR_PIN, 0);

    epd_lcd_deinit();
}

static void epd_board_set_ctrl(epd_ctrl_state_t* state, const epd_ctrl_state_t* const mask) {
    /* Handle output enable changes */
    if (mask->ep_output_enable) {
        gpio_set_level(OE_PIN, state->ep_output_enable ? 1 : 0);
    }
}

static void epd_board_poweron(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Powering on display");

    /* Enable main power first */
    gpio_set_level(PWR_PIN, 1);

    /* Wait for power to stabilize */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Enable output */
    gpio_set_level(OE_PIN, 1);

    /* Update state */
    state->ep_stv = true;
    state->ep_mode = false;
    state->ep_output_enable = true;
    state->ep_sth = true;
}

static void epd_board_poweroff(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Powering off display");

    /* Disable output first */
    gpio_set_level(OE_PIN, 0);

    state->ep_stv = false;
    state->ep_output_enable = false;
    state->ep_mode = false;
    state->ep_sth = false;

    /* Small delay before cutting power */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Cut main power */
    gpio_set_level(PWR_PIN, 0);
}

static float epd_board_ambient_temperature() {
    /* TODO: Could read from BMI270 temperature sensor */
    return 20.0f;
}

const EpdBoardDefinition epd_board_m5papers3 = {
    .init = epd_board_init,
    .deinit = epd_board_deinit,
    .set_ctrl = epd_board_set_ctrl,
    .poweron = epd_board_poweron,
    .poweroff = epd_board_poweroff,
    .get_temperature = epd_board_ambient_temperature,
    // No hardware VCOM control path on this board yet - a non-null callback that doesn't touch
    // hardware would let epd_set_vcom() complete without changing anything on the panel.
    .set_vcom = NULL,
    .gpio_set_direction = NULL,
    .gpio_read = NULL,
    .gpio_write = NULL,
};
