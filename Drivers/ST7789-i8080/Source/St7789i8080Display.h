#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_st7789.h>
#include <array>
#include <memory>
#include <functional>
#include <mutex>

class St7789i8080Display : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        // Pin configuration
        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t wrPin;
        gpio_num_t rdPin;
        std::array<gpio_num_t, 8> dataPins;
        gpio_num_t resetPin;
        gpio_num_t backlightPin;
        
        // Display resolution configuration
        uint16_t horizontalResolution;
        uint16_t verticalResolution;
        
        // Bus configuration
        unsigned int pixelClockFrequency = 16 * 1000 * 1000;   // 16MHz default
        size_t busWidth = 8;                                  // 8-bit bus
        size_t transactionQueueDepth = 40;
        size_t bufferSize = 0;                                 // Will be calculated if 0
        
        // LCD command/parameter configuration
        int lcdCmdBits = 8;
        int lcdParamBits = 8;
        
        // DC line level configuration
        struct {
            bool dcIdleLevel = 0;
            bool dcCmdLevel = 0;
            bool dcDummyLevel = 0;
            bool dcDataLevel = 1;
        } dcLevels;
        
        // Bus flags
        struct {
            bool csActiveHigh = false;
            bool reverseColorBits = false;
            bool swapColorBytes = true;
            bool pclkActiveNeg = false;
            bool pclkIdleLow = false;
        } flags;
        
        // Display configuration
        int gapX = 0;
        int gapY = 0;
        bool swapXY = false;
        bool mirrorX = false;
        bool mirrorY = false;
        bool invertColor = true;
        
        // Additional features
        std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr;
        std::function<void(uint8_t)> backlightDutyFunction = nullptr;

        // Basic constructor - requires resolution to be set separately
        Configuration(gpio_num_t cs, gpio_num_t dc, gpio_num_t wr, gpio_num_t rd,
                      std::array<gpio_num_t, 8> data, gpio_num_t rst, gpio_num_t bl)
            : csPin(cs), dcPin(dc), wrPin(wr), rdPin(rd),
              dataPins(data), resetPin(rst), backlightPin(bl),
              horizontalResolution(0), verticalResolution(0) {}  // Initialize to 0
        
        // Method to calculate buffer size after resolution is set
        void calculateBufferSize() {
            if (bufferSize == 0 && horizontalResolution > 0 && verticalResolution > 0) {
                bufferSize = horizontalResolution * verticalResolution / 10;
            }
        }
        
        // Validation method
        bool isValid() const {
            return horizontalResolution > 0 && verticalResolution > 0;
        }
    };

private:
    Configuration configuration;
    esp_lcd_i80_bus_handle_t i80BusHandle = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* lvglDisplay = nullptr;
    std::shared_ptr<std::mutex> lock;
    uint8_t* buf1 = nullptr;

    // Internal initialization methods
    void sendInitCommands();
    bool createI80Bus();
    bool createPanelIO();
    bool createPanel();

public:
    explicit St7789i8080Display(const Configuration& config);
    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }
    std::string getName() const override { return "I8080 ST7789"; }
    std::string getDescription() const override { return "I8080-based ST7789 display"; }
    
    // Lifecycle
    bool start() override;
    bool stop() override;
    bool startLvgl() override;
    bool stopLvgl() override;
    
    // Capabilities
    bool supportsLvgl() const override { return true; }
    bool supportsDisplayDriver() const override { return false; }
    bool supportsBacklightDuty() const override { return configuration.backlightDutyFunction != nullptr; }
    
    // Touch and backlight
    std::shared_ptr<tt::hal::touch::TouchDevice> getTouchDevice() override { return configuration.touch; }
    std::shared_ptr<tt::hal::display::DisplayDriver> getDisplayDriver() override { return nullptr; }
    
    void setBacklightDuty(uint8_t backlightDuty) override {
        if (configuration.backlightDutyFunction != nullptr) {
            configuration.backlightDutyFunction(backlightDuty);
        }
    }

    // Hardware access methods
    esp_lcd_panel_io_handle_t getIoHandle() const { return ioHandle; }
    esp_lcd_panel_handle_t getPanelHandle() const { return panelHandle; }
    
    // Resolution access methods
    uint16_t getHorizontalResolution() const { return configuration.horizontalResolution; }
    uint16_t getVerticalResolution() const { return configuration.verticalResolution; }
};

// Factory function for registration
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();