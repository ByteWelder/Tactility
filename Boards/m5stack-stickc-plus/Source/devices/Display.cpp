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
    St7789Display::Configuration panel_configuration = {
        .horizontalResolution = LCD_HORIZONTAL_RESOLUTION,
        .verticalResolution = LCD_VERTICAL_RESOLUTION,
        .gapX = 52,
        .gapY = 40,
        .swapXY = false,
        .mirrorX = false,
        .mirrorY = false,
        .invertColor = true,
        .bufferSize = LCD_BUFFER_SIZE,
        .touch = nullptr,
        .backlightDutyFunction = setBrightness,
        .resetPin = LCD_PIN_RESET,
        .lvglSwapBytes = false
    };

    auto spi_configuration = std::make_shared<St7789Display::SpiConfiguration>(St7789Display::SpiConfiguration {
        .spiHostDevice = LCD_SPI_HOST,
        .csPin = LCD_PIN_CS,
        .dcPin = LCD_PIN_DC,
        .pixelClockFrequency = 40'000'000,
        .transactionQueueDepth = 10
    });

    return std::make_shared<St7789Display>(panel_configuration, spi_configuration);
}
