#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_types.h>
#include <lvgl.h>
#include <functional>

namespace tt::hal::display {

class I80Display final : public tt::hal::display::DisplayDevice {
public:
    enum class PanelType {
        ST7789,
        ILI9341,

        ILI9486,
        ILI9488,
        CUSTOM
    };
    
    enum class RotationMode {
        ROTATE_0,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270,
        SOFTWARE
    };
    
    // Structure for custom initialization commands
    struct InitCommand {
        uint8_t cmd;
        const uint8_t* data;
        size_t dataSize;
        uint32_t delayMs;
    };

    // Default configuration values for common displays
    static constexpr uint16_t CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT = 10;

    class Configuration {
    public:
        Configuration(
            gpio_num_t dcPin,
            gpio_num_t wrPin,
            const int* dataPins,
            unsigned int horizontalResolution,
            unsigned int verticalResolution,
            PanelType panelType = PanelType::ST7789,
            unsigned int busWidth = 8,
            gpio_num_t csPin = GPIO_NUM_NC
        ) : dcPin(dcPin),
            wrPin(wrPin),
            csPin(csPin),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            panelType(panelType),
            busWidth(busWidth) {
            // Copy data pins based on busWidth
            for (int i = 0; i < busWidth && i < 16; i++) {
                this->dataPins[i] = dataPins[i];
            }
            for (int i = busWidth; i < 16; i++) {
                this->dataPins[i] = GPIO_NUM_NC; // Fill unused pins
            }
        }

        // Basic pins configuration
        gpio_num_t dcPin;
        gpio_num_t wrPin;
        gpio_num_t csPin = GPIO_NUM_NC;
        gpio_num_t resetPin = GPIO_NUM_NC;
        gpio_num_t backlightPin = GPIO_NUM_NC;
        int dataPins[16]; // Max 16-bit bus, filled based on busWidth
        
        // Display parameters
        unsigned int horizontalResolution;
        unsigned int verticalResolution;
        PanelType panelType;
        unsigned int busWidth;
        
        // Bus configuration
        unsigned int pixelClockFrequency = 20'000'000; // Default 20 MHz
        size_t transactionQueueDepth = 10;
        size_t maxTransferBytes = 0;  // 0 for default (uses DEFAULT_MAX_TRANSFER_BYTES)
        size_t dmaBurstSize = 0;      // 0 for default (uses DEFAULT_DMA_BURST_SIZE)
        int cmdBits = 8;
        int paramBits = 8;
        
        // Panel orientation and color settings
        RotationMode rotationMode = RotationMode::ROTATE_0;
        bool invertColor = false;
        lcd_rgb_element_order_t rgbElementOrder = LCD_RGB_ELEMENT_ORDER_RGB;
        uint8_t dataEndian = 0;  // 0 for big endian, 1 for little endian
        int bitsPerPixel = 16;   // Usually 16 (RGB565) or 24 (RGB888)
        
        // Display control flags
        bool csActiveHigh = false;
        bool resetActiveHigh = false;
        bool reverseColorBits = false;
        bool swapColorBytes = false;
        bool pclkActiveNeg = false;
        bool pclkIdleLow = false;
        bool backlightActiveLow = false;
        
        // Buffer and LVGL configuration
        uint16_t drawBufferHeight = 0;  // 0 for default
        bool useDoubleBuffer = false;
        size_t transactionSize = 0;  // 0 for default (entire buffer)
        bool useDmaBuffer = true;
        bool useSpiRamBuffer = false;
        bool swapBytesLVGL = false;
        bool useFullRefresh = false;
        bool useDirectMode = false;
        
        // Custom initialization
        const InitCommand* customInitCommands = nullptr;
        size_t customInitCommandsCount = 0;
        void* vendorConfig = nullptr;
        std::function<esp_err_t(esp_lcd_panel_io_handle_t, const esp_lcd_panel_dev_config_t*, esp_lcd_panel_handle_t*)> customPanelSetup = nullptr;
        
        // Transaction callbacks
        std::function<void(I80Display*, void*)> onTransactionDone = nullptr;
        std::function<void(lv_display_t*)> displayCallbacks = nullptr;
        
        // Debug options
        bool debugFlushCalls = false;
        bool debugMemory = false;
        bool runDisplayTest = false;
        
        // Optimization flags
        bool useBatchCommands = true;
        bool supportsBrightnessCommand = false;
        
        // Touch integration
        std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr;
    };

private:
    std::unique_ptr<Configuration> configuration;
    esp_lcd_i80_bus_handle_t i80Bus = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;
    
    // Private initialization methods
    bool validateConfiguration();
    bool initializeGPIO();
    bool initializeI80Bus();
    bool initializePanelIO();
    bool initializePanel();
    bool configurePanel();
    bool setupLVGLDisplay();
    bool cleanupResources();
    bool setBatchArea(const lv_area_t* area);
    void runDisplayTest();
    void logMemoryStats(const char* stage);

public:
    explicit I80Display(std::unique_ptr<Configuration> inConfiguration)
        : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    ~I80Display() {
        stop();
    }

    std::string getName() const final {
        switch (configuration->panelType) {
            case PanelType::ST7789: return "ST7789-I80";
            case PanelType::ILI9341: return "ILI9341-I80";
            case PanelType::ILI9486: return "ILI9486-I80";
            case PanelType::ILI9488: return "ILI9488-I80";
            case PanelType::CUSTOM: return "Custom-I80";
            default: return "Unknown-I80";
        }
    }

    std::string getDescription() const final { return "I80-based display"; }

    bool start() final;
    bool stop() final;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() final {
        return configuration->touch;
    }

    void setBacklightDuty(uint8_t backlightDuty) final {
        setBrightness(backlightDuty);
    }

    bool supportsBacklightDuty() const final {
        return configuration->backlightPin != GPIO_NUM_NC || 
               configuration->supportsBrightnessCommand;
    }

    void setGammaCurve(uint8_t index) final override;
    uint8_t getGammaCurveCount() const final { return 4; }
    
    bool setBrightness(uint8_t brightness);
    bool setInvertColor(bool invert);

    lv_display_t* _Nullable getLvglDisplay() const final { return displayHandle; }
};

} // namespace tt::hal::display
