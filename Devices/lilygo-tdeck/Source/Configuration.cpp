#include "devices/Display.h"
#include "devices/KeyboardBacklight.h"
#include "devices/Power.h"
#include "devices/Sdcard.h"
#include "devices/TdeckKeyboard.h"
#include "devices/TrackballDevice.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/Device.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/Log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

bool initBoot();
void initI2cDevices();

using namespace tt::hal;

constexpr auto* TAG = "TDeckConfig";

static std::vector<std::shared_ptr<Device>> createDevices() {
    return {
        createPower(),
        createDisplay(),
        std::make_shared<TdeckKeyboard>(),
        std::make_shared<KeyboardBacklightDevice>(),
        std::make_shared<TrackballDevice>(),
        createSdCard()
    };
}

void initI2cDevices() {
    // Defer I2C device startup to avoid heap corruption during early boot
    // Use a one-shot FreeRTOS timer to delay initialization
    static TimerHandle_t initTimer = xTimerCreate(
        "I2CInit",
        pdMS_TO_TICKS(500), // 500ms delay
        pdFALSE, // One-shot
        nullptr,
        [](TimerHandle_t timer) {
            TT_LOG_I(TAG, "Starting deferred I2C devices");
            
            // Start keyboard backlight device
            auto kbBacklight = tt::hal::findDevice("Keyboard Backlight");
            if (kbBacklight) {
                TT_LOG_I(TAG, "%s starting", kbBacklight->getName().c_str());
                auto kbDevice = std::static_pointer_cast<KeyboardBacklightDevice>(kbBacklight);
                if (kbDevice->start()) {
                    TT_LOG_I(TAG, "%s started", kbBacklight->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "%s start failed", kbBacklight->getName().c_str());
                }
            }
            
            // Small delay between I2C device inits to avoid concurrent transactions
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Start trackball device
            auto trackball = tt::hal::findDevice("Trackball");
            if (trackball) {
                TT_LOG_I(TAG, "%s starting", trackball->getName().c_str());
                auto tbDevice = std::static_pointer_cast<TrackballDevice>(trackball);
                if (tbDevice->start()) {
                    TT_LOG_I(TAG, "%s started", trackball->getName().c_str());
                } else {
                    TT_LOG_E(TAG, "%s start failed", trackball->getName().c_str());
                }
            }
            
            TT_LOG_I(TAG, "Deferred I2C devices completed");
        }
    );
    
    if (initTimer != nullptr) {
        xTimerStart(initTimer, 0);
    }
}

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .initI2cDevices = initI2cDevices,
    .createDevices = createDevices,
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_18,
                .scl_io_num = GPIO_NUM_8,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        i2c::Configuration {
            .name = "External",
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_43,
                .scl_io_num = GPIO_NUM_44,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    },
    .spi {
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_41,
                .miso_io_num = GPIO_NUM_38,
                .sclk_io_num = GPIO_NUM_40,
                .quadwp_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = LCD_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    },
    .uart {
        uart::Configuration {
            .name = "Grove",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_44,
            .txPin = GPIO_NUM_43,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 38400,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .rx_flow_ctrl_thresh = 0,
                .source_clk = UART_SCLK_DEFAULT,
                .flags = {
                    .allow_pd = 0,
                    .backup_before_sleep = 0,
                }
            }
        }
    }
};
