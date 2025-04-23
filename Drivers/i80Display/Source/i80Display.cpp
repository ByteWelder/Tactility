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

// Add other panel support (future)


#define TAG "i80display"

namespace tt::hal::display {

// Transaction done callback for panel IO
static bool transactionDoneCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* event_data, void* user_ctx) {
    auto* self = static_cast<I80Display*>(user_ctx);
    // Provide a public/protected accessor for configuration if needed, or make this a friend if necessary
    if (self && self->getOnTransactionDone()) {
        self->getOnTransactionDone()(self, io);
    }
    return true;
}

namespace {
    // Panel command constants
    constexpr uint8_t LCD_CMD_SLEEP_OUT = 0x11;
    constexpr uint8_t LCD_CMD_DISPLAY_ON = 0x29;

    
    // Display initialization delay constants
    constexpr uint32_t SLEEP_OUT_DELAY_MS = 120;
    constexpr uint32_t DISPLAY_ON_DELAY_MS = 50;
    
    // Default DMA configuration
    constexpr size_t DEFAULT_MAX_TRANSFER_BYTES = 32768; // Increased from 16384
    constexpr size_t DEFAULT_DMA_BURST_SIZE = 128;       // Increased from 64
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

    // Step 4: Configure panel
    if (!configurePanel()) {
        TT_LOG_E(TAG, "Failed to configure panel");
        cleanupResources();
        return false;
    }

    // Step 5: Set up LVGL display
    if (!setupLVGLDisplay()) {
        TT_LOG_E(TAG, "Failed to set up LVGL display");
        cleanupResources();
        return false;
    }

    // Optional: Run display test if configured
    if (configuration->runDisplayTest) {
        runDisplayTest();
    }

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
    // Log memory stats before allocation
    logMemoryStats("before bus initialization");
    
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = configuration->dcPin,
        .wr_gpio_num = configuration->wrPin,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            configuration->dataPins[0], configuration->dataPins[1], 
            configuration->dataPins[2], configuration->dataPins[3],
            configuration->dataPins[4], configuration->dataPins[5], 
            configuration->dataPins[6], configuration->dataPins[7],
            configuration->dataPins[8], configuration->dataPins[9], 
            configuration->dataPins[10], configuration->dataPins[11],
            configuration->dataPins[12], configuration->dataPins[13], 
            configuration->dataPins[14], configuration->dataPins[15]
        },
        .bus_width = configuration->busWidth,
        .max_transfer_bytes = configuration->maxTransferBytes > 0 ? 
                              configuration->maxTransferBytes : DEFAULT_MAX_TRANSFER_BYTES,
        .dma_burst_size = configuration->dmaBurstSize > 0 ? 
                         configuration->dmaBurstSize : DEFAULT_DMA_BURST_SIZE,
        .sram_trans_align = DEFAULT_SRAM_ALIGN,
    };
    
    RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_config, &i80Bus));
    
    // Log memory stats after allocation
    logMemoryStats("after bus initialization");
    
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
    // Reset panel if reset pin is configured
    if (configuration->resetPin != GPIO_NUM_NC) {
        RETURN_ON_ERROR(esp_lcd_panel_reset(panelHandle));
    }
    
    // Initialize the panel
    RETURN_ON_ERROR(esp_lcd_panel_init(panelHandle));
    
    // Apply custom initialization commands if provided
    if (configuration->customInitCommands && configuration->customInitCommandsCount > 0) {
        for (size_t i = 0; i < configuration->customInitCommandsCount; i++) {
            const auto& cmd = configuration->customInitCommands[i];
            RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, cmd.cmd, 
                                                     cmd.data, cmd.dataSize));
            if (cmd.delayMs > 0) {
                vTaskDelay(pdMS_TO_TICKS(cmd.delayMs));
            }
        }
    } else {
        // Default initialization sequence (mimic LovyanGFX)
        uint8_t colmod[] = {static_cast<uint8_t>(configuration->bitsPerPixel == 16 ? 0x05 : 0x06)}; // RGB565 or RGB666
        
        // Sleep Out
        RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_SLEEP_OUT, nullptr, 0));
        vTaskDelay(pdMS_TO_TICKS(SLEEP_OUT_DELAY_MS));
        
        // COLMOD - Color pixel format
        RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_COLMOD, colmod, 1));
        
        // Display ON
        RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_DISPLAY_ON, nullptr, 0));
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_ON_DELAY_MS));
    }
    
    // Set panel orientation
    if (configuration->rotationMode != RotationMode::SOFTWARE) {
        bool swapXY = false;
        bool mirrorX = false;
        bool mirrorY = false;
        
        switch (configuration->rotationMode) {
            case RotationMode::ROTATE_0:
                break;
            case RotationMode::ROTATE_90:
                swapXY = true;
                mirrorY = true;
                break;
            case RotationMode::ROTATE_180:
                mirrorX = true;
                mirrorY = true;
                break;
            case RotationMode::ROTATE_270:
                swapXY = true;
                mirrorX = true;
                break;
            default:
                break;
        }
        
        RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panelHandle, swapXY));
        RETURN_ON_ERROR(esp_lcd_panel_mirror(panelHandle, mirrorX, mirrorY));
    } else {
        // Avoid hardware rotation to prevent conflicts when using software rotation
        RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panelHandle, false));
        RETURN_ON_ERROR(esp_lcd_panel_mirror(panelHandle, false, false));
    }
    
    // Set color inversion if needed
    RETURN_ON_ERROR(esp_lcd_panel_invert_color(panelHandle, configuration->invertColor));
    
    // Turn on the display
    RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panelHandle, true));
    
    // Turn on backlight if configured
    if (configuration->backlightPin != GPIO_NUM_NC) {
        gpio_set_level(configuration->backlightPin, 
                      configuration->backlightActiveLow ? 0 : 1);
    }
    
    return true;
}

bool tt::hal::display::I80Display::setupLVGLDisplay() {
    uint32_t buffer_size = configuration->horizontalResolution * 
                          (configuration->drawBufferHeight > 0 ? 
                           configuration->drawBufferHeight : 
                           CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT);
    
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = configuration->useDoubleBuffer,
        .trans_size = configuration->transactionSize,
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = false,
        .rotation = { // This corresponds to LVGL's rotation
            .swap_xy = (configuration->rotationMode == RotationMode::ROTATE_90 || 
                       configuration->rotationMode == RotationMode::ROTATE_270),
            .mirror_x = (configuration->rotationMode == RotationMode::ROTATE_180 || 
                        configuration->rotationMode == RotationMode::ROTATE_270),
            .mirror_y = (configuration->rotationMode == RotationMode::ROTATE_180 || 
                        configuration->rotationMode == RotationMode::ROTATE_90)
        },
        .color_format = (configuration->bitsPerPixel == 16) ? 
                        LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB888,
        .flags = {
            .buff_dma = configuration->useDmaBuffer,
            .buff_spiram = configuration->useSpiRamBuffer,
            .sw_rotate = configuration->rotationMode == RotationMode::SOFTWARE,
            .swap_bytes = configuration->swapBytesLVGL,
            .full_refresh = configuration->useFullRefresh,
            .direct_mode = configuration->useDirectMode
        }
    };
    
    displayHandle = lvgl_port_add_disp(&disp_cfg);
    if (!displayHandle) {
        TT_LOG_E(TAG, "Failed to initialize LVGL display");
        return false;
    }
    
    // Set up flush callback
    lv_display_set_user_data(displayHandle, this);
    lv_display_set_flush_cb(displayHandle, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
        auto* self = static_cast<tt::hal::display::I80Display*>(lv_display_get_user_data(disp));
        
        // Always log the flush area and pointer
        TT_LOG_I(TAG, "LVGL flush: area=%p x1=%ld y1=%ld x2=%ld y2=%ld", (void*)area, (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2);
        // Drawing optimization - batch commands if supported by the controller
        if (self->configuration->useBatchCommands && self->setBatchArea(area)) {
            // If batch area setup succeeded, we can use optimized drawing
            TT_LOG_I(TAG, "Batch draw: area=%p x1=%ld y1=%ld x2=%ld y2=%ld", (void*)area, (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2);
            esp_lcd_panel_draw_bitmap(self->panelHandle,
                                     area->x1, area->y1,
                                     area->x2 + 1, area->y2 + 1, px_map);
        } else {
            // Fallback to regular drawing
            TT_LOG_I(TAG, "Regular draw: area=%p x1=%ld y1=%ld x2=%ld y2=%ld", (void*)area, (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2);
            if (area->x1 > area->x2 || area->y1 > area->y2) {
                TT_LOG_E(TAG, "Invalid area for draw_bitmap: x1=%ld y1=%ld x2=%ld y2=%ld (area=%p)", (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2, (void*)area);
                lv_display_flush_ready(disp);
                return;
            }
            TT_LOG_I(TAG, "draw_bitmap: x1=%ld y1=%ld x2=%ld y2=%ld (area=%p)", (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2, (void*)area);
            esp_lcd_panel_draw_bitmap(self->panelHandle,
                                     area->x1, area->y1, 
                                     area->x2 + 1, area->y2 + 1, px_map);
        }
        
        lv_display_flush_ready(disp);
    });

    // Register additional callbacks if configured
    if (configuration->displayCallbacks) {
        configuration->displayCallbacks(displayHandle);
    }
    
    return true;
}

void tt::hal::display::I80Display::runDisplayTest() {
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
}

bool tt::hal::display::I80Display::setBatchArea(const lv_area_t* area) {
    // This is a method to optimize drawing by setting the area once
    // and then sending pixel data without repeating the area command.
    // Implemented for common display controllers like ILI9341, ST7789, etc.
    
    // This usually involves sending CASET and PASET commands
    // Example implementation for ILI9341/ST7789:
    uint8_t caset[4] = {
        static_cast<uint8_t>((area->x1 >> 8) & 0xFF),
        static_cast<uint8_t>(area->x1 & 0xFF),
        static_cast<uint8_t>((area->x2 >> 8) & 0xFF),
        static_cast<uint8_t>(area->x2 & 0xFF)
    };
    
    uint8_t paset[4] = {
        static_cast<uint8_t>((area->y1 >> 8) & 0xFF),
        static_cast<uint8_t>(area->y1 & 0xFF),
        static_cast<uint8_t>((area->y2 >> 8) & 0xFF),
        static_cast<uint8_t>(area->y2 & 0xFF)
    };
    
    // Send column address set
    if (esp_lcd_panel_io_tx_param(ioHandle, 0x2A, caset, 4) != ESP_OK) {
        return false;
    }
    
    // Send page address set
    if (esp_lcd_panel_io_tx_param(ioHandle, 0x2B, paset, 4) != ESP_OK) {
        return false;
    }
    
    // Start memory write
    if (esp_lcd_panel_io_tx_param(ioHandle, 0x2C, nullptr, 0) != ESP_OK) {
        return false;
    }
    
    return true;
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
            return;
    }
    
    const uint8_t param[] = { gamma_curve };
    if (esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_GAMSET, param, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma curve");
        return;
    }
    // Success, do nothing further
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
