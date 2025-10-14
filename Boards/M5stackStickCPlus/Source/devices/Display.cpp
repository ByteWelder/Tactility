#include "Display.h"

#include "Power.h"

#include <St7789Display.h>
#include <bitset>

constexpr auto* TAG = "StickCPlus";

static void setBacklightOn(bool on) {
    const auto axp = getAxp192();
    const auto* driver = axp->getAxp192();
    uint8_t state;
    if (axp192_read(driver, AXP192_DCDC13_LDO23_CONTROL, &state) != AXP192_OK) {
        TT_LOG_I(TAG, "Failed to read LCD brightness state");
        return;
    }
    std::bitset<8> new_state = state;
    if (new_state[2] != on) {
        new_state[2] = on;
        const auto new_state_long = new_state.to_ulong();
        axp192_write(driver, AXP192_DCDC13_LDO23_CONTROL, static_cast<uint8_t>(new_state_long)); // Display on/off
    }
}

static void setBrightness(uint8_t brightness) {
    const auto axp = getAxp192();
    if (brightness)
    {
        brightness = (((brightness >> 1) + 8) / 13) + 5;
        setBacklightOn(true);
        axp192_write(axp->getAxp192(), AXP192_LDO23_VOLTAGE, brightness << 4); // Display brightness
    }
    else
    {
        setBacklightOn(false);
    }
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto configuration = std::make_unique<St7789Display::Configuration>(
        LCD_SPI_HOST,
        LCD_PIN_CS,
        LCD_PIN_DC,
        LCD_HORIZONTAL_RESOLUTION,
        LCD_VERTICAL_RESOLUTION,
        nullptr,
        false,
        false,
        false,
        true,
        LCD_DRAW_BUFFER_SIZE,
        52,
        40
    );

    configuration->pixelClockFrequency = 40'000'000;
    configuration->resetPin = LCD_PIN_RESET;

    configuration->backlightDutyFunction = setBrightness;
    const auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
