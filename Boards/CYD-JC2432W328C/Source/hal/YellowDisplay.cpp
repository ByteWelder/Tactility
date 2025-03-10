#include "YellowDisplay.h"
#include "Cst816Touch.h"
#include "YellowDisplayConstants.h"

#include <St7789Display.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        I2C_NUM_0,
        240,
        320
    );

    return std::make_shared<Cst816sTouch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<St7789Display::Configuration>(
        JC2432W328C_LCD_SPI_HOST,
        JC2432W328C_LCD_PIN_CS,
        JC2432W328C_LCD_PIN_DC,
        JC2432W328C_LCD_HORIZONTAL_RESOLUTION,
        JC2432W328C_LCD_VERTICAL_RESOLUTION,
        touch,
        false,
        false,
        false,
        false,
        JC2432W328C_LCD_DRAW_BUFFER_SIZE
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<St7789Display>(std::move(configuration));
}
