#include "UnPhoneDisplay.h"
#include "UnPhoneDisplayConstants.h"
#include "UnPhoneTouch.h"
#include "UnPhoneFeatures.h"

#include <Tactility/Log.h>

#include <esp_err.h>
#include <hx8357/disp_spi.h>
#include <hx8357/hx8357.h>

#define TAG "unphone_display"
#define BUFFER_SIZE (UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_DRAW_BUFFER_HEIGHT * LV_COLOR_DEPTH / 8)

extern std::shared_ptr<UnPhoneFeatures> unPhoneFeatures;

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
    buffer = (uint8_t*)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
    assert(buffer != nullptr);

    lv_display_set_buffers(
        displayHandle,
        buffer,
        nullptr,
        BUFFER_SIZE,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(displayHandle, hx8357_flush);

    if (displayHandle != nullptr) {
        TT_LOG_I(TAG, "Finished");
        unPhoneFeatures->setBacklightPower(true);
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

    heap_caps_free(buffer);
    buffer = nullptr;

    return true;
}

std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable UnPhoneDisplay::createTouch() {
    return ::createTouch();
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    return std::make_shared<UnPhoneDisplay>();
}
