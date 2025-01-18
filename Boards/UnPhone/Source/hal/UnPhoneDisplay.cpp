#include "UnPhoneDisplay.h"
#include "UnPhoneDisplayConstants.h"
#include "UnPhoneTouch.h"
#include "Log.h"

#include <TactilityCore.h>

#include "esp_err.h"
#include "hx8357/hx8357.h"
#include "hx8357/disp_spi.h"

#define TAG "unphone_display"

bool UnPhoneDisplay::start() {
    TT_LOG_I(TAG, "Starting");

    disp_spi_add_device(SPI2_HOST);

    hx8357_init();
    hx8357_set_rotation(2);

    displayHandle = lv_display_create(UNPHONE_LCD_HORIZONTAL_RESOLUTION, UNPHONE_LCD_VERTICAL_RESOLUTION);
    static uint8_t buf1[UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_VERTICAL_RESOLUTION / 10 * UNPHONE_LCD_BITS_PER_PIXEL / 8];
    static uint8_t buf2[UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_VERTICAL_RESOLUTION / 10 * UNPHONE_LCD_BITS_PER_PIXEL / 8];
    lv_display_set_buffers(
        displayHandle,
        buf1,
        buf2,
        sizeof(buf1),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_color_format(displayHandle, LV_COLOR_FORMAT_RGB565);
    lv_display_set_physical_resolution(displayHandle, UNPHONE_LCD_HORIZONTAL_RESOLUTION, UNPHONE_LCD_VERTICAL_RESOLUTION);
    lv_display_set_flush_cb(displayHandle, hx8357_flush);

    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool UnPhoneDisplay::stop() {
    tt_assert(displayHandle != nullptr);

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
