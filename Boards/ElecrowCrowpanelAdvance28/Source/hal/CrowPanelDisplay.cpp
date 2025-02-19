#include "CrowPanelDisplay.h"
#include "CrowPanelDisplayConstants.h"
#include "CrowPanelTouch.h"

#include <St7789Display.h>
#include <PwmBacklight.h>

#include <Tactility/Log.h>

#define TAG "crowpanel_display"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = std::make_shared<CrowPanelTouch>();

    auto configuration = std::make_unique<St7789Display::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        320,
        240,
        touch
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<St7789Display>(std::move(configuration));
}
