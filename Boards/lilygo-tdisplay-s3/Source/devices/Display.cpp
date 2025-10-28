#include "Display.h"
#include <St7789i8080Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto config = St7789i8080Display::Configuration(
        GPIO_NUM_6,   // CS
        GPIO_NUM_7,   // DC
        GPIO_NUM_8,   // WR
        GPIO_NUM_9,   // RD
        { GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42,
          GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48 }, // D0..D7
        GPIO_NUM_5,   // RST
        GPIO_NUM_38   // BL
    );
    
    // Configure resolution for Lilygo T-Display S3
    config.horizontalResolution = 170;
    config.verticalResolution = 320;
    config.gapX = 35;
    config.pixelClockFrequency = 10 * 1000 * 1000; // 10MHz for better stability
    
    auto display = std::make_shared<St7789i8080Display>(config);
    return display;
}
