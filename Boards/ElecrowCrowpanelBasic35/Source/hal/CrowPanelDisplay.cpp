#include "CrowPanelDisplay.h"
#include "CrowPanelDisplayConstants.h"

#include <Ili9488Display.h>
#include <Xpt2046Touch.h>

#include <PwmBacklight.h>

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_TOUCH_PIN_CS,
        320,
        480,
        true,
        true,
        true
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    /**
     * This display is mirrored on X, but that doesn't seem to work with the driver.
     * Instead, we swap XY, which does work. That results in a landscape image.
     */
    auto configuration = std::make_unique<Ili9488Display::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        480,
        320,
        touch,
        true,
        false,
        false
    );

    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili9488Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
