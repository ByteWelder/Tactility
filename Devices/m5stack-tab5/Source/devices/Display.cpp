#include "Display.h"
#include "Ili9881cDisplay.h"

#include <Gt911Touch.h>
#include <PwmBacklight.h>
#include <Tactility/Logger.h>
#include <Tactility/Mutex.h>

constexpr auto LCD_PIN_RESET = GPIO_NUM_0;  // Match P4 EV board reset line
constexpr auto LCD_PIN_BACKLIGHT = GPIO_NUM_22;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        273,
        1280,
        false,  // swapXY
        false,  // mirrorX
        false,  // mirrorY
        GPIO_NUM_NC, // reset pin
        GPIO_NUM_23 // interrupt pin
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    // Initialize PWM backlight
    if (!driver::pwmbacklight::init(LCD_PIN_BACKLIGHT, 20000, LEDC_TIMER_1, LEDC_CHANNEL_0)) {
        tt::Logger("Tab5").warn("Failed to initialize backlight");
    }

    auto touch = createTouch();

    auto configuration = std::make_shared<EspLcdConfiguration>(EspLcdConfiguration {
        .horizontalResolution = 720,
        .verticalResolution = 1280,
        .gapX = 0,
        .gapY = 0,
        .monochrome = false,
        .swapXY = false,
        .mirrorX = false,
        .mirrorY = false,
        .invertColor = false,
        .bufferSize = 0, // 0 = default (1/10 of screen)
        .touch = touch,
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = LCD_PIN_RESET,
        .lvglColorFormat = LV_COLOR_FORMAT_RGB565,
        .lvglSwapBytes = false,
        .rgbElementOrder = LCD_RGB_ELEMENT_ORDER_RGB,
        .bitsPerPixel = 16
    });

    const auto display = std::make_shared<Ili9881cDisplay>(configuration);
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
