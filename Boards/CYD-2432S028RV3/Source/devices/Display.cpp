#include "Display.h"
#include "Xpt2046SoftSpi.h"
#include <St7789Display.h>
#include <PwmBacklight.h>

constexpr auto* TAG = "CYD";

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046SoftSpi::Configuration>(
        CYD_TOUCH_MOSI_PIN,
        CYD_TOUCH_MISO_PIN,
        CYD_TOUCH_SCK_PIN,
        CYD_TOUCH_CS_PIN,
        CYD2432S028RV3_LCD_HORIZONTAL_RESOLUTION, // 240
        CYD2432S028RV3_LCD_VERTICAL_RESOLUTION, // 320
        false, // swapXY
        true, // mirrorX
        false // mirrorY
    );

    // Allocate the driver
    auto touch = std::make_shared<Xpt2046SoftSpi>(std::move(configuration));
    
    // Start the driver
    if (!touch->start()) {
        ESP_LOGE(TAG, "Touch driver start failed");
        return nullptr;
    }

    return touch;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<St7789Display::Configuration>(
        CYD2432S028RV3_LCD_SPI_HOST,
        CYD2432S028RV3_LCD_PIN_CS,
        CYD2432S028RV3_LCD_PIN_DC,
        CYD2432S028RV3_LCD_HORIZONTAL_RESOLUTION,
        CYD2432S028RV3_LCD_VERTICAL_RESOLUTION,
        touch,
        false, // swapXY
        false, // mirrorX
        false, // mirrorY
        false, // invertColor
        CYD2432S028RV3_LCD_DRAW_BUFFER_SIZE
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
