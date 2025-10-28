#include "Display.h"
#include "St7789i8080Display.h"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    // Create configuration with explicit resolution
    auto config = St7789i8080Display::Configuration(
        GPIO_NUM_6,   // CS
        GPIO_NUM_7,   // DC
        GPIO_NUM_8,   // WR
        GPIO_NUM_9,   // RD
        { GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42,
          GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48 }, // D0..D7
        GPIO_NUM_5,   // RST
        GPIO_NUM_38,  // BL
        170,          // Horizontal resolution
        320           // Vertical resolution
    );
    
    config.gapX = 35;  // ST7789 has a 35 pixel gap on X axis
    config.pixelClockFrequency = 10 * 1000 * 1000; // 10MHz for better stability
    
    auto display = std::make_shared<St7789i8080Display>(config);
    return display;
}
