#include "bootstrap.h"

#include "M5Unified.hpp"
#include "config.h"
#include "log.h"

m5::IMU_Class& Imu = M5.Imu;
m5::Power_Class& Power = M5.Power;
m5::RTC8563_Class& Rtc = M5.Rtc;
m5::Touch_Class& Touch = M5.Touch;

m5::Speaker_Class& Speaker = M5.Speaker;

m5::Button_Class& BtnPWR = M5.BtnPWR;

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "core2_bootstrap"

static bool init_i2c() {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CORE2_I2C_PIN_SDA,
        .scl_io_num = CORE2_I2C_PIN_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master = {
            .clk_speed = 100000
        },
        .clk_flags = 0
    };

    if (i2c_param_config(CORE2_TOUCH_I2C_PORT, &i2c_conf) != ESP_OK) {
        TT_LOG_E(TAG, "i2c config failed");
        return false;
    }

    if (i2c_driver_install(CORE2_TOUCH_I2C_PORT, i2c_conf.mode, 0, 0, 0) != ESP_OK) {
        TT_LOG_E(TAG, "i2c driver install failed");
        return false;
    }

    return true;
}

static bool init_spi2() {
    const spi_bus_config_t bus_config = {
        .mosi_io_num = CORE2_SPI2_PIN_MOSI,
        .miso_io_num = CORE2_SPI2_PIN_MISO,
        .sclk_io_num = CORE2_SPI2_PIN_SCLK,
        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = CORE2_SPI2_TRANSACTION_LIMIT,
        .flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, "SPI bus init failed");
        return false;
    }

    return true;
}

static void log_power_status() {
    TT_LOG_I(
        TAG,
        "Battery: level = %ld, voltage = %.1f, charging = %s",
        M5.Power.getBatteryLevel(),
        (float)M5.Power.getBatteryVoltage() / 1000.0f,
        M5.Power.isCharging() ? "true" : "false"
    );
}

bool core2_bootstrap() {
    TT_LOG_I(TAG, "Initializing M5Unified");
    M5.begin();
    // For models with EPD : refresh control
    M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.
    log_power_status();

    TT_LOG_I(TAG, "Initializing I2C");
    if (!init_i2c()) {
        return false;
    }

    TT_LOG_I(TAG, "Initializing SPI");
    if (!init_spi2()) {
        return false;
    }

    return true;
}

#ifdef __cplusplus
}
#endif
