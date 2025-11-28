#include "Display.h"

#include <PwmBacklight.h>
#include <Ft6x36Touch.h>
#include <St7796i8080Display.h>

constexpr auto LCD_HORIZONTAL_RESOLUTION = 320;
constexpr auto LCD_VERTICAL_RESOLUTION = 480;

std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Ft6x36Touch::Configuration>(
        I2C_NUM_0,
        GPIO_NUM_7,
        LCD_HORIZONTAL_RESOLUTION,
        LCD_VERTICAL_RESOLUTION
    );

    auto touch = std::make_shared<Ft6x36Touch>(std::move(configuration));
    return std::static_pointer_cast<tt::hal::touch::TouchDevice>(touch);
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto configuration = St7796i8080Display::Configuration(
        GPIO_NUM_NC, //CS
        GPIO_NUM_0,  //RS
        GPIO_NUM_47, //WR
        { GPIO_NUM_9, GPIO_NUM_46, GPIO_NUM_3, GPIO_NUM_8,
          GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_15 }, // D0..D7
        GPIO_NUM_4, //RESET
        GPIO_NUM_45 //BL
    );

    configuration.mirrorX = true;
    configuration.horizontalResolution = LCD_HORIZONTAL_RESOLUTION;
    configuration.verticalResolution = LCD_VERTICAL_RESOLUTION;
    configuration.touch = createTouch();
    configuration.backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<St7796i8080Display>(configuration);
    return display;
}
