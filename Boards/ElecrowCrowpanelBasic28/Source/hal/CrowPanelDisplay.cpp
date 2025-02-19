#include "CrowPanelDisplay.h"
#include "CrowPanelDisplayConstants.h"
#include "CrowPanelTouch.h"
#include "Ili934xDisplay.h"

#include <PwmBacklight.h>

#include <Tactility/Log.h>

#define TAG "crowpanel_display"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        240,
        320,
        touch
    );

    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
