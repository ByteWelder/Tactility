#include "SoftSPI.h"
#include <esp_log.h>
#include <rom/ets_sys.h>

static const char* TAG = "SoftSPI";

SoftSPI::SoftSPI(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sck)
    : miso_(miso), mosi_(mosi), sck_(sck), cs_pin_(GPIO_NUM_NC) {}

void SoftSPI::begin() {
    gpio_set_direction(miso_, GPIO_MODE_INPUT);
    gpio_set_direction(mosi_, GPIO_MODE_OUTPUT);
    gpio_set_direction(sck_, GPIO_MODE_OUTPUT);
    gpio_set_level(sck_, 0);
    ESP_LOGI(TAG, "SoftSPI initialized: MISO=%d, MOSI=%d, SCK=%d", miso_, mosi_, sck_);
}

uint8_t SoftSPI::transfer(uint8_t data) {
    uint8_t result = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(mosi_, (data >> i) & 1);
        ets_delay_us(10);  // Slow for XPT2046
        gpio_set_level(sck_, 1);
        ets_delay_us(10);
        result |= (gpio_get_level(miso_) << i);
        gpio_set_level(sck_, 0);
        ets_delay_us(10);
    }
    return result;
}
