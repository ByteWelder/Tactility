#include "YellowDisplay.h"
#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "St7789-i8080Display.h"
#include <Tactility/Log.h>
#include <inttypes.h>

#define TAG "YellowDisplay"

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    TT_LOG_I(TAG, "Creating touch device");
    auto touch = createYellowTouch();
    if (!touch) {
        TT_LOG_E(TAG, "Failed to create touch device");
    }
    return touch;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    TT_LOG_I(TAG, "Creating ST7789 I8080 display");

    // Create touch device
    auto touch = createTouch();
    if (!touch) {
        TT_LOG_E(TAG, "Touch device creation failed, proceeding without touch");
    }

    // Create configuration
    auto configuration = std::make_unique<St7789I8080Display::Configuration>(
        CYD_2432S022C_LCD_PIN_WR,        // WR pin
        CYD_2432S022C_LCD_PIN_DC,        // DC pin
        CYD_2432S022C_LCD_PIN_CS,        // CS pin
        CYD_2432S022C_LCD_PIN_RST,       // RST pin
        CYD_2432S022C_LCD_PIN_BACKLIGHT, // Backlight pin
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, // Horizontal resolution
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,   // Vertical resolution
        touch,                           // Touch device
        CYD_2432S022C_LCD_PCLK_HZ,       // Pixel clock
        false,                           // swapXY
        false,                           // mirrorX
        false,                           // mirrorY
        true,                            // invertColor (for IPS ST7789)
        0,                               // bufferSize (default: 1/10 screen size)
        CYD_2432S022C_LCD_BACKLIGHT_ON_LEVEL == 1 // backlightOnLevel
    );

    // Configure 8-bit data bus
    configuration->setDataPins8Bit(
        CYD_2432S022C_LCD_PIN_D0,
        CYD_2432S022C_LCD_PIN_D1,
        CYD_2432S022C_LCD_PIN_D2,
        CYD_2432S022C_LCD_PIN_D3,
        CYD_2432S022C_LCD_PIN_D4,
        CYD_2432S022C_LCD_PIN_D5,
        CYD_2432S022C_LCD_PIN_D6,
        CYD_2432S022C_LCD_PIN_D7
    );

    // Log configuration details
    TT_LOG_I(TAG, "Configuration: %dx%d, PCLK=%" PRIu32 " Hz, Backlight GPIO=%d, Bus width=%" PRIu8,
             configuration->horizontalResolution,
             configuration->verticalResolution,
             configuration->pixelClockHz,
             configuration->pin_backlight,
             configuration->busWidth);

    // Create and return display
    auto display = std::make_shared<St7789I8080Display>(std::move(configuration));
    TT_LOG_I(TAG, "Display created successfully");
    return display;
}
