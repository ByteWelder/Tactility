#pragma once

#include "../Gpio.h"

#ifdef ESP_PLATFORM
#include <driver/spi_common.h>
#else

#define SPI_HOST_MAX 3

typedef int spi_host_device_t;
struct spi_bus_config_t {
    gpio_num_t miso_io_num;
    gpio_num_t mosi_io_num;
    gpio_num_t sclk_io_num;
};
struct spi_common_dma_t {};

#endif