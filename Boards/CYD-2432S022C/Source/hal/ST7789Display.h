#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

// Constants for the display
namespace cyd_2432s022c {
    static constexpr int HORIZONTAL_RESOLUTION = 240;
    static constexpr int VERTICAL_RESOLUTION = 320;
    static constexpr int DRAW_BUFFER_HEIGHT = VERTICAL_RESOLUTION / 10;
    static constexpr int DRAW_BUFFER_SIZE = HORIZONTAL_RESOLUTION * DRAW_BUFFER_HEIGHT;
}

class ST7789Display : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        int width;
        int height;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
    };

    ST7789Display(std::unique_ptr<Configuration> config);
    ~ST7789Display() override = default;

    void init() override;
    void flush(const lv_area_t *area, const lv_color_t *color_p) override;
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }
    std::shared_ptr<tt::hal::touch::TouchDevice> get_touch() const override { return config_->touch; }

private:
    void write_byte(uint8_t data);
    void set_address_window(int x, int y, int w, int h);

    std::unique_ptr<Configuration> config_;

    // Pin definitions (from schematic)
    static constexpr int PIN_D0 = 15;
    static constexpr int PIN_D1 = 13;
    static constexpr int PIN_D2 = 12;
    static constexpr int PIN_D3 = 14;
    static constexpr int PIN_D4 = 27;
    static constexpr int PIN_D5 = 25;
    static constexpr int PIN_D6 = 33;
    static constexpr int PIN_D7 = 32;
    static constexpr int PIN_WR = 4;
    static constexpr int PIN_RD = 2;
    static constexpr int PIN_RS = 16;
    static constexpr int PIN_CS = 17;
};
