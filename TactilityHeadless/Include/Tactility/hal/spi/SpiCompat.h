#pragma once

#ifdef ESP_PLATFORM
#include <driver/spi_common.h>
#else

#include <cstdint>

enum spi_host_device_t {
    SPI1_HOST = 0,
    SPI2_HOST = 1,
    SPI3_HOST = 2,
    SPI_HOST_MAX,
};

enum spi_common_dma_t {
    SPI_DMA_DISABLED = 0, ///< No DMA
    SPI_DMA_CH1 = 1, ///< DMA, select DMA Channel 1
    SPI_DMA_CH2 = 2, ///< DMA, select DMA Channel 2
    SPI_DMA_CH_AUTO = 3, ///< DMA, channel selected by driver
};

enum esp_intr_cpu_affinity_t {
    ESP_INTR_CPU_AFFINITY_AUTO,
    ESP_INTR_CPU_AFFINITY_0,
    ESP_INTR_CPU_AFFINITY_1,
};

struct spi_bus_config_t {
    union {
        int mosi_io_num;
        int data0_io_num;
    };
    union {
        int miso_io_num;
        int data1_io_num;
    };
    int sclk_io_num;
    union {
        int quadwp_io_num;
        int data2_io_num;
    };
    union {
        int quadhd_io_num;
        int data3_io_num;
    };
    int data4_io_num;
    int data5_io_num;
    int data6_io_num;
    int data7_io_num;
    bool data_io_default_level;
    int max_transfer_sz;
    uint32_t flags;
    esp_intr_cpu_affinity_t isr_cpu_id;
    int intr_flags;
};

#endif