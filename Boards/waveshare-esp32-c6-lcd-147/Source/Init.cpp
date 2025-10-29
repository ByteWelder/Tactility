#include "devices/Display.h"

#include <PwmBacklight.h>

bool initBoot() {
    // Initialize backlight with 5 kHz frequency (as per demo code)
    return driver::pwmbacklight::init(LCD_PIN_BACKLIGHT, 5000);
}
