#include "CYD2432S028R.h"
#include "hal/YellowDisplay.h"
#include "hal/YellowDisplayConstants.h"
#include <Tactility/lvgl/LvglSync.h>
#include <PwmBacklight.h>

#define CYD_SPI_TRANSFER_SIZE_LIMIT (240 * 320 / 4 * 2)

bool initBoot() {
    return driver::pwmbacklight::init(CYD_BACKLIGHT_PIN);
}

const tt::hal::Configuration cyd_2432s028r_config = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createYellowSdCard(),
    .power = nullptr,
    .i2c = {},
    .spi = {
        tt::hal::spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_13,
                .miso_io_num = GPIO_NUM_12,
                .sclk_io_num = GPIO_NUM_14,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = CYD_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        }
    }
};

extern "C" void app_main() {
    tt::start(&cyd_2432s028r_config);
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Touch me!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, [](lv_event_t* e) {
        ESP_LOGI("LVGL", "Label touched!");
    }, LV_EVENT_PRESSED, nullptr);
}
