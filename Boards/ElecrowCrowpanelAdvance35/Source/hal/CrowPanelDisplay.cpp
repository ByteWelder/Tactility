#include "CrowPanelDisplay.h"
#include "CrowPanelDisplayConstants.h"

#include <Ft5x06Touch.h>
#include <PwmBacklight.h>
#include <Ili9488Display.h>

#define TAG "crowpanel_display"

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note for future changes: Reset pin is 48 and interrupt pin is 47
    auto configuration = std::make_unique<Ft5x06Touch::Configuration>(
        I2C_NUM_0,
        CROWPANEL_LCD_HORIZONTAL_RESOLUTION,
        CROWPANEL_LCD_VERTICAL_RESOLUTION,
        true,
        true,
        false
    );

    return std::make_shared<Ft5x06Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = nullptr;//createTouch();

    auto configuration = std::make_unique<Ili9488Display::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        CROWPANEL_LCD_HORIZONTAL_RESOLUTION,
        CROWPANEL_LCD_VERTICAL_RESOLUTION,
        touch,
        false,
        false,
        false,
        true
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<Ili9488Display>(std::move(configuration));
}
