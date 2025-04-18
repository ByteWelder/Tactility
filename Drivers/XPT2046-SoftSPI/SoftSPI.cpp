#include "SoftSPI.h"
#include <esp_log.h>
#include <rom/ets_sys.h>

static const char* TAG = "SoftSPI";

SoftSPI::SoftSPI(const Config& config)
    : miso_pin_(config.miso_pin),
      mosi_pin_(config.mosi_pin),
      sck_pin_(config.sck_pin),
      cs_pin_(config.cs_pin),
      delay_us_(config.delay_us),
      post_command_delay_us_(2) // Default to 2us, can be tuned
{
}


bool SoftSPI::begin() {
    // Configure pins
    gpio_config_t miso_config = {
        .pin_bit_mask = (1ULL << miso_pin_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << mosi_pin_) | (1ULL << sck_pin_) | (1ULL << cs_pin_),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    // Apply configurations
    esp_err_t ret = gpio_config(&miso_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MISO pin: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = gpio_config(&output_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure output pins: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Set initial pin states
    gpio_set_level(sck_pin_, 0);
    gpio_set_level(mosi_pin_, 0);
    gpio_set_level(cs_pin_, 1);  // CS inactive high
    
    ESP_LOGI(TAG, "SoftSPI initialized: MISO=%d, MOSI=%d, SCK=%d, CS=%d", 
             miso_pin_, mosi_pin_, sck_pin_, cs_pin_);
    return true;
}

uint8_t SoftSPI::transfer(uint8_t data) {
    uint8_t result = 0;
    
    // Shift out and in each bit, MSB first
    for (int i = 7; i >= 0; i--) {
        // Set data bit
        gpio_set_level(mosi_pin_, (data >> i) & 1);
        ets_delay_us(delay_us_);
        
        // Clock high
        gpio_set_level(sck_pin_, 1);
        ets_delay_us(delay_us_);
        
        // Read data bit
        result |= (gpio_get_level(miso_pin_) << i);
        
        // Clock low
        gpio_set_level(sck_pin_, 0);
        ets_delay_us(delay_us_);
    }
    
    return result;
}

void SoftSPI::cs_low() {
    gpio_set_level(cs_pin_, 0);
}

void SoftSPI::cs_high() {
    gpio_set_level(cs_pin_, 1);
}
