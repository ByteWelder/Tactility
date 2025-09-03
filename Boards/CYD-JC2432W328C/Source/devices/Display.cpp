#include "Display.h"

#include <Cst816Touch.h>
#include <PwmBacklight.h>
#include <St7789Display.h>

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

    auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
