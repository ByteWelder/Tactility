#include "i80Display.h"
#include "Tactility/Log.h"
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_ili9341.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_commands.h>
// Add other panel support (future)


#define TAG "i80display"

namespace tt::hal::display {

// Transaction done callback for panel IO
bool transactionDoneCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* event_data, void* user_ctx) {
    auto* self = static_cast<I80Display*>(user_ctx);
    if (self && self->configuration->onTransactionDone) {
        self->configuration->onTransactionDone(io, event_data, user_ctx);
    }
    return true;
}

namespace {
    // Default gamma curve for ST7789: curve 1 (0x01)
    constexpr uint8_t DEFAULT_GAMMA_CURVE = 0x01;

    // Display initialization delay constants
    constexpr uint32_t SLEEP_OUT_DELAY_MS = 120;
    constexpr uint8_t DISPLAY_ON_DELAY_MS = 50;
    
    // Default DMA configuration
    constexpr size_t DEFAULT_SRAM_ALIGN = 64;
    
    // Error handling helper
    #define RETURN_ON_ERROR(x) do { \
        esp_err_t err = (x); \
        if (err != ESP_OK) { \
            TT_LOG_E(TAG, "%s failed with error 0x%x: %s", #x, err, esp_err_to_name(err)); \
            return false; \
        } \
    } while(0)
}


bool I80Display::start() {
    TT_LOG_I(TAG, "Starting I80 Display");
    
    // Check configuration validity
    if (!validateConfiguration()) {
        TT_LOG_E(TAG, "Invalid display configuration");
        return false;
    }

    // Initialize GPIO pins first (for reset and backlight if provided)
    if (!initializeGPIO()) {
        TT_LOG_E(TAG, "Failed to initialize GPIO pins");
        return false;
    }

    // Step 1: Initialize I80 bus with improved error handling
    if (!initializeI80Bus()) {
        TT_LOG_E(TAG, "Failed to initialize I80 bus");
        return false;
    }

    // Step 2: Initialize panel I/O
    if (!initializePanelIO()) {
        TT_LOG_E(TAG, "Failed to initialize panel I/O");
        cleanupResources();
        return false;
    }

    // Step 3: Initialize panel
    if (!initializePanel()) {
        TT_LOG_E(TAG, "Failed to initialize panel");
        cleanupResources();
        return false;
    }

    // Step 4: Configure panel with explicit settings
    if (!configurePanel()) {
        TT_LOG_E(TAG, "Failed to configure panel");
        cleanupResources();
        return false;
    }

    // Step 5: Set up LVGL display with color format handling
    if (!setupLVGLDisplay()) {
        TT_LOG_E(TAG, "Failed to set up LVGL display");
        cleanupResources();
        return false;
    }

    // Optional: Run display test if configured
    // if (configuration->runDisplayTest) {
    //     runDisplayTest();
    // }
    
    TT_LOG_I(TAG, "I80 Display initialization completed successfully");
    return true;
}

bool I80Display::validateConfiguration() {
    // Validate essential pins
    if (configuration->dcPin == GPIO_NUM_NC || 
        configuration->wrPin == GPIO_NUM_NC) {
        TT_LOG_E(TAG, "DC or WR pins not configured");
        return false;
    }
    
    // Validate data pins based on bus width
    for (int i = 0; i < configuration->busWidth; i++) {
        if (configuration->dataPins[i] == GPIO_NUM_NC) {
            TT_LOG_E(TAG, "Data pin %d not configured", i);
            return false;
        }
    }
    
    // Validate resolution
    if (configuration->horizontalResolution == 0 || 
        configuration->verticalResolution == 0) {
        TT_LOG_E(TAG, "Invalid display resolution");
        return false;
    }
    
    // Validate pixel clock frequency
    if (configuration->pixelClockFrequency < 1000000) { // 1MHz minimum
        TT_LOG_W(TAG, "Pixel clock frequency might be too low: %u Hz", 
                 configuration->pixelClockFrequency);
    }
    
    return true;
}

bool I80Display::initializeGPIO() {
    // Configure Reset pin if specified
    if (configuration->resetPin != GPIO_NUM_NC) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << configuration->resetPin);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        
        RETURN_ON_ERROR(gpio_config(&io_conf));
        
        // Initial reset state
        RETURN_ON_ERROR(gpio_set_level(configuration->resetPin, 0));
        vTaskDelay(pdMS_TO_TICKS(10));
        RETURN_ON_ERROR(gpio_set_level(configuration->resetPin, 1));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Configure Backlight pin if specified
    if (configuration->backlightPin != GPIO_NUM_NC) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << configuration->backlightPin);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        
        RETURN_ON_ERROR(gpio_config(&io_conf));
        
        // Initialize backlight off, will turn on after full initialization
        RETURN_ON_ERROR(gpio_set_level(configuration->backlightPin, 
                                      configuration->backlightActiveLow ? 1 : 0));
    }
    
    return true;
}

bool I80Display::initializeI80Bus() {
    TT_LOG_I(TAG, "Initializing I80 bus");
    
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = configuration->dcPin,
        .wr_gpio_num = configuration->wrPin,
        .clk_src = LCD_CLK_SRC_DEFAULT, // Moved clk_src here
        .data_gpio_nums = {
            configuration->dataPins[0],
            configuration->dataPins[1],
            configuration->dataPins[2],
            configuration->dataPins[3],
            configuration->dataPins[4],
            configuration->dataPins[5],
            configuration->dataPins[6],
            configuration->dataPins[7]
        },
        .bus_width = 8,  // Explicitly set bus width
        .max_transfer_bytes = DEFAULT_MAX_TRANSFER_BYTES,
        .dma_burst_size = DEFAULT_DMA_BURST_SIZE,
        // psram_trans_align and sram_trans_align omitted as they default to 0
    };

    RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_config, &i80Bus));
    return true;
}

bool I80Display::initializePanelIO() {
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = configuration->csPin,
        .pclk_hz = configuration->pixelClockFrequency,
        .trans_queue_depth = configuration->transactionQueueDepth,
        .on_color_trans_done = configuration->onTransactionDone ? transactionDoneCallback : nullptr,
        .user_ctx = configuration->onTransactionDone ? this : nullptr,
        .lcd_cmd_bits = configuration->cmdBits > 0 ? configuration->cmdBits : 8,
        .lcd_param_bits = configuration->paramBits > 0 ? configuration->paramBits : 8,
        .dc_levels = { 
            .dc_idle_level = 0, 
            .dc_cmd_level = 0, 
            .dc_dummy_level = 0, 
            .dc_data_level = 1 
        },
        .flags = { 
            .cs_active_high = configuration->csActiveHigh ? 1u : 0u, 
            .reverse_color_bits = configuration->reverseColorBits ? 1u : 0u, 
            .swap_color_bytes = configuration->swapColorBytes ? 1u : 0u,
            .pclk_active_neg = configuration->pclkActiveNeg ? 1u : 0u, 
            .pclk_idle_low = configuration->pclkIdleLow ? 1u : 0u 
        }
    };

    
    RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(i80Bus, &io_config, &ioHandle));
    
    return true;
}

bool tt::hal::display::I80Display::initializePanel() {
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->resetPin,
        .rgb_ele_order = configuration->rgbElementOrder,
        .data_endian = configuration->dataEndian == 0 ? 
                      LCD_RGB_DATA_ENDIAN_BIG : LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = static_cast<uint32_t>(configuration->bitsPerPixel > 0 ? 
                         configuration->bitsPerPixel : 16),
        .flags = { 
            .reset_active_high = configuration->resetActiveHigh ? 1u : 0u 
        },
        .vendor_config = configuration->vendorConfig
    };
    
    esp_err_t ret = ESP_OK;
    
    // Create panel based on panel type
    switch (configuration->panelType) {
        case PanelType::ST7789:
            ret = esp_lcd_new_panel_st7789(ioHandle, &panel_config, &panelHandle);
            break;
        case PanelType::ILI9341:
            ret = esp_lcd_new_panel_ili9341(ioHandle, &panel_config, &panelHandle);
            break;

        case PanelType::CUSTOM:
            if (configuration->customPanelSetup) {
                ret = configuration->customPanelSetup(ioHandle, &panel_config, &panelHandle);
                break;
            }
            // Explicitly fall through if no custom setup provided
            [[fallthrough]];
        default:
            TT_LOG_E(TAG, "Unsupported panel type");
            return false;
    }
    
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel: %s", esp_err_to_name(ret));
        return false;
    }
    
    return true;
}

bool tt::hal::display::I80Display::configurePanel() {
    TT_LOG_I(TAG, "Configuring panel");
    
    // Set color format and bit depth
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->resetPin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // Explicitly set RGB order
        .bits_per_pixel = 16,
    };

    // Initialize panel
    RETURN_ON_ERROR(esp_lcd_panel_reset(panelHandle));
    RETURN_ON_ERROR(esp_lcd_panel_init(panelHandle));

    // ST7789 Initialization Sequence
    // 1. Exit Sleep Mode
    RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_SLPOUT, nullptr, 0));
    vTaskDelay(pdMS_TO_TICKS(SLEEP_OUT_DELAY_MS));

    // 2. Set Color Mode to 16-bit (65k colors)
    constexpr uint8_t COLOR_MODE_16BIT = 0x05;
    RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_COLMOD, &COLOR_MODE_16BIT, 1));

    // 3. Enable Color Inversion (for IPS displays)
    RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_INVON, nullptr, 0));

    // 4. Set Memory Access Control (optional, adjust as needed)
    constexpr uint8_t MEMORY_ACCESS_CONTROL = 0x08;  // Adjust based on display orientation
    RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_MADCTL, &MEMORY_ACCESS_CONTROL, 1));

    // 5. Turn on Display
    RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_DISPON, nullptr, 0));
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_ON_DELAY_MS));

    // Gamma Correction
    if (configuration->supportsGammaCorrection) {
        setGammaCurve(0);  // Simply call without checking return value
    }

    return true;
}
bool tt::hal::display::I80Display::setupLVGLDisplay() {
    TT_LOG_I(TAG, "Setting up LVGL display");

    if (!panelHandle || !configuration) {
        TT_LOG_E(TAG, "Invalid panelHandle or configuration");
        return false;
    }

    // Map Configuration to lvgl_port_display_cfg_t
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,  // Use ioHandle instead of panel
        .panel_handle = panelHandle,  // Add panel handle
        .control_handle = nullptr,    // No control handle needed
        .buffer_size = configuration->horizontalResolution * 
                     (configuration->drawBufferHeight > 0 ? 
                      configuration->drawBufferHeight : 
                      DEFAULT_DRAW_BUFFER_HEIGHT) * 
                     (configuration->bitsPerPixel / 8),
        .double_buffer = configuration->useDoubleBuffer,
        .trans_size = 0,  // No transfer size needed
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false
        },
        .color_format = configuration->bitsPerPixel == 16 ? LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB888,
        .flags = {
            .buff_dma = static_cast<unsigned int>(configuration->useDmaBuffer),
            .buff_spiram = static_cast<unsigned int>(configuration->useSpiRamBuffer),
            .sw_rotate = static_cast<unsigned int>(configuration->rotationMode == RotationMode::SOFTWARE),
            .swap_bytes = static_cast<unsigned int>(configuration->swapBytesLVGL),
            .full_refresh = 0,
            .direct_mode = 0
        }
    };

    // Debug logging
    if (configuration->debugMemory) {
        TT_LOG_I(TAG, "disp_cfg: io_handle=%p, hres=%" PRIu32 ", vres=%" PRIu32 ", buffer_size=%" PRIu32 ", color_format=%d",
                 disp_cfg.io_handle, disp_cfg.hres, disp_cfg.vres, 
                 disp_cfg.buffer_size, disp_cfg.color_format);
    }

    // Create LVGL display
    displayHandle = lvgl_port_add_disp(&disp_cfg);
    if (!displayHandle) {
        TT_LOG_E(TAG, "Failed to create LVGL display");
        return false;
    }

    // Allocate draw buffers
    size_t draw_buffer_sz = disp_cfg.buffer_size;
    uint32_t malloc_caps = configuration->useDmaBuffer ? MALLOC_CAP_DMA : MALLOC_CAP_DEFAULT;
    if (configuration->useSpiRamBuffer) {
        malloc_caps |= MALLOC_CAP_SPIRAM;
    }

    void* buf1 = heap_caps_malloc(draw_buffer_sz, malloc_caps);
    void* buf2 = configuration->useDoubleBuffer ? heap_caps_malloc(draw_buffer_sz, malloc_caps) : nullptr;

    if (!buf1 || (configuration->useDoubleBuffer && !buf2)) {
        TT_LOG_E(TAG, "Failed to allocate draw buffers");
        heap_caps_free(buf1);
        heap_caps_free(buf2);
        lvgl_port_remove_disp(displayHandle);
        displayHandle = nullptr;
        return false;
    }

    // Initialize LVGL draw buffers
    lv_display_set_buffers(displayHandle, buf1, buf2, draw_buffer_sz, 
                          configuration->useFullRefresh ? LV_DISPLAY_RENDER_MODE_FULL : LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(displayHandle, panelHandle);

    // Apply custom display callbacks if provided
    if (configuration->displayCallbacks) {
        configuration->displayCallbacks(displayHandle);
    }

    // Set flush callback with color format handling
    lv_display_set_flush_cb(displayHandle, [](lv_display_t* disp, const lv_area_t* area, uint8_t* color_map) {
        esp_lcd_panel_handle_t panel_handle = static_cast<esp_lcd_panel_handle_t>(lv_display_get_user_data(disp));
        int offsetx1 = area->x1;
        int offsetx2 = area->x2;
        int offsety1 = area->y1;
        int offsety2 = area->y2;

        // Swap RGB bytes if configured
        if (lv_display_get_color_format(disp) == LV_COLOR_FORMAT_RGB565) {
            lv_draw_sw_rgb565_swap(color_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
        }

        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    });

    // Log memory stats after allocation
    logMemoryStats("after LVGL display setup");

    TT_LOG_I(TAG, "LVGL display setup completed successfully");
    return true;
}

bool I80Display::runDisplayTest() {
    const uint32_t width = configuration->horizontalResolution;
    const uint32_t height = configuration->verticalResolution;
    const size_t bufferSize = width * height * sizeof(uint16_t);
    
    TT_LOG_I(TAG, "Running display test - red screen");
    
    // Log memory stats before allocation
    logMemoryStats("before test buffer allocation");
    
    // Allocate buffer with DMA capability
    uint16_t* testBuffer = static_cast<uint16_t*>(
        heap_caps_malloc(bufferSize, MALLOC_CAP_DMA));
    
    if (testBuffer) {
        // Fill with red color (RGB565)
        for (uint32_t i = 0; i < width * height; i++) {
            testBuffer[i] = 0xF800;
        }
        
        // Draw to panel
        esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, width, height, testBuffer);
        
        // Small delay to see the test pattern
        vTaskDelay(pdMS_TO_TICKS(500));
        
        // Free the buffer
        heap_caps_free(testBuffer);
        
        TT_LOG_I(TAG, "Display test completed");
    } else {
        TT_LOG_E(TAG, "Failed to allocate memory for display test");
    }
    
    // Log memory stats after allocation
    logMemoryStats("after test buffer deallocation");
    
    return true;
}

bool tt::hal::display::I80Display::setBatchArea(const lv_area_t* area) {
    // Optimized batch area setting for ST7789
    
    // Column Address Set (CASET)
    const uint8_t caset[4] = {
        static_cast<uint8_t>((area->x1 >> 8) & 0xFF),  // Start column high byte
        static_cast<uint8_t>(area->x1 & 0xFF),         // Start column low byte
        static_cast<uint8_t>((area->x2 >> 8) & 0xFF),  // End column high byte
        static_cast<uint8_t>(area->x2 & 0xFF)          // End column low byte
    };
    
    // Page Address Set (RASET)
    const uint8_t raset[4] = {
        static_cast<uint8_t>((area->y1 >> 8) & 0xFF),  // Start page high byte
        static_cast<uint8_t>(area->y1 & 0xFF),         // Start page low byte
        static_cast<uint8_t>((area->y2 >> 8) & 0xFF),  // End page high byte
        static_cast<uint8_t>(area->y2 & 0xFF)          // End page low byte
    };
    
    // Use separate transactions to minimize buffer usage
    esp_err_t ret = ESP_OK;
    
    // Column Address Set
    ret |= esp_lcd_panel_io_tx_param(ioHandle, 
        LCD_CMD_CASET,  // Column Address Set command
        caset, 
        sizeof(caset)
    );
    
    // Page Address Set
    ret |= esp_lcd_panel_io_tx_param(ioHandle, 
        LCD_CMD_RASET,  // Page Address Set command
        raset, 
        sizeof(raset)
    );
    
    // Memory Write command
    ret |= esp_lcd_panel_io_tx_param(ioHandle, 
        LCD_CMD_RAMWR,  // Memory Write command
        nullptr, 
        0
    );
    
    return ret == ESP_OK;
}

void tt::hal::display::I80Display::setGammaCurve(uint8_t index) {
    uint8_t gamma_curve;
    switch (index) {
        case 0: gamma_curve = 0x01; break; // Gamma curve 1
        case 1: gamma_curve = 0x04; break; // Gamma curve 2
        case 2: gamma_curve = 0x02; break; // Gamma curve 3
        case 3: gamma_curve = 0x08; break; // Gamma curve 4
        default: 
            TT_LOG_E(TAG, "Invalid gamma curve index: %u", index);
            return;  // Just return if invalid index
    }
    
    const uint8_t param[] = { gamma_curve };
    esp_err_t result = esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_GAMSET, param, 1);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma curve: %s", esp_err_to_name(result));
        // No return value, just log the error
    }
}

bool tt::hal::display::I80Display::setBrightness(uint8_t brightness) {
    // Implement brightness control if hardware supports it
    if (configuration->backlightPin != GPIO_NUM_NC) {
        // For simple GPIO backlight control
        bool isOn = brightness > 0;
        return gpio_set_level(configuration->backlightPin, 
                             configuration->backlightActiveLow ? !isOn : isOn) == ESP_OK;
    }
    
    // For panels that support brightness control via commands
    if (configuration->supportsBrightnessCommand) {
        const uint8_t param[] = { brightness };
        return esp_lcd_panel_io_tx_param(ioHandle, 0x51, param, 1) == ESP_OK;
    }
    
    return false;
}

bool tt::hal::display::I80Display::setInvertColor(bool invert) {
    if (esp_lcd_panel_invert_color(panelHandle, invert) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set color inversion");
        return false;
    }
    return true;
}

bool tt::hal::display::I80Display::stop() {
    TT_LOG_I(TAG, "Stopping I80 Display");
    
    if (!displayHandle) {
        TT_LOG_W(TAG, "Display not started");
        return true;
    }
    
    // Remove LVGL display
    lvgl_port_remove_disp(displayHandle);
    displayHandle = nullptr;
    
    // Turn off display and backlight
    if (panelHandle) {
        esp_lcd_panel_disp_on_off(panelHandle, false);
    }
    
    if (configuration->backlightPin != GPIO_NUM_NC) {
        gpio_set_level(configuration->backlightPin, 
                      configuration->backlightActiveLow ? 1 : 0);
    }
    
    return cleanupResources();
}

bool tt::hal::display::I80Display::cleanupResources() {
    bool success = true;
    
    // Delete panel
    if (panelHandle) {
        if (esp_lcd_panel_del(panelHandle) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to delete panel");
            success = false;
        }
        panelHandle = nullptr;
    }
    
    // Delete panel IO
    if (ioHandle) {
        if (esp_lcd_panel_io_del(ioHandle) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to delete panel IO");
            success = false;
        }
        ioHandle = nullptr;
    }
    
    // Delete I80 bus
    if (i80Bus) {
        if (esp_lcd_del_i80_bus(i80Bus) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to delete I80 bus");
            success = false;
        }
        i80Bus = nullptr;
    }
    
    TT_LOG_I(TAG, "I80 Display stopped %s", success ? "successfully" : "with errors");
    return success;
}

void tt::hal::display::I80Display::logMemoryStats(const char* stage) {
    if (configuration->debugMemory) {
        TT_LOG_I(TAG, "Memory stats %s:", stage);
        TT_LOG_I(TAG, "  DMA heap free: %lu", 
                static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DMA)));
        TT_LOG_I(TAG, "  Largest DMA block free: %lu", 
                static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)));
        TT_LOG_I(TAG, "  Total heap free: %lu", 
                static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT)));
    }
}

} // namespace tt::hal::display
