#include "UnPhoneDisplay.h"
#include "UnPhoneDisplayConstants.h"
#include "UnPhoneTouch.h"
#include "Log.h"

#include <TactilityCore.h>

#include "UnPhoneFeatures.h"
#include "esp_err.h"
#include "hx8357/disp_spi.h"
#include "hx8357/hx8357.h"

#define TAG "unphone_display"
#define BUFFER_SIZE (UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_DRAW_BUFFER_HEIGHT * LV_COLOR_DEPTH / 8)

extern UnPhoneFeatures unPhoneFeatures;

bool UnPhoneDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    disp_spi_add_device(SPI2_HOST);

    hx8357_reset(GPIO_NUM_46);
    hx8357_init(UNPHONE_LCD_PIN_DC);
    uint8_t madctl = (1U << MADCTL_BIT_INDEX_COLUMN_ADDRESS_ORDER);
    hx8357_set_madctl(madctl);

    displayHandle = lv_display_create(UNPHONE_LCD_HORIZONTAL_RESOLUTION, UNPHONE_LCD_VERTICAL_RESOLUTION);
    lv_display_set_physical_resolution(displayHandle, UNPHONE_LCD_HORIZONTAL_RESOLUTION, UNPHONE_LCD_VERTICAL_RESOLUTION);
    lv_display_set_color_format(displayHandle, LV_COLOR_FORMAT_NATIVE);

    // TODO malloc to use SPIRAM
    static auto* buffer1 = (uint8_t*)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    static auto* buffer2 = (uint8_t*)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    assert(buffer1 != nullptr);
    assert(buffer2 != nullptr);

    lv_display_set_buffers(
        displayHandle,
        buffer1,
        buffer2,
        BUFFER_SIZE,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(displayHandle, hx8357_flush);

    if (displayHandle != nullptr) {
        TT_LOG_I(TAG, "Finished");
        unPhoneFeatures.setBacklightPower(true);
        return true;
    } else {
        TT_LOG_I(TAG, "Failed");
        return false;
    }
}

bool UnPhoneDisplay::stop() {
    assert(displayHandle != nullptr);

    lv_display_delete(displayHandle);
    displayHandle = nullptr;

    return true;
}

tt::hal::Touch* _Nullable UnPhoneDisplay::createTouch() {
    return static_cast<tt::hal::Touch*>(new UnPhoneTouch());
}

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new UnPhoneDisplay());
}
