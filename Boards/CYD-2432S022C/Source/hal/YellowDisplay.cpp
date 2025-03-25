#include "YellowDisplay.h"
#include "CYD2432S022CConstants.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_log.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "PwmBacklight.h"

#define TAG "YellowDisplay"

namespace tt::hal::display {

YellowDisplay::YellowDisplay(std::unique_ptr<Configuration> config)
    : config(std::move(config)), panelHandle(nullptr), lvglDisplay(nullptr), isStarted(false) {
}

YellowDisplay::~YellowDisplay() {
    if (isStarted) {
        stop();
    }
}

bool YellowDisplay::start() {
    if (isStarted) {
        ESP_LOGW(TAG, "Display already started");
        return true;
    }

    initialize();
    if (!lvglDisplay) {
        ESP_LOGE(TAG, "Failed to initialize display");
        deinitialize();
        return false;
    }

    if (!driver::pwmbacklight::init(config->backlightPin)) {
        ESP_LOGE(TAG, "Failed to initialize PWM backlight");
        deinitialize();
        return false;
    }

    isStarted = true;
    ESP_LOGI(TAG, "Display started successfully");

    setBacklightDuty(tt::app::display::getBacklightDuty());
    setRotation(tt::app::display::getRotation());

    return true;
}

bool YellowDisplay::stop() {
    if (!isStarted) {
        ESP_LOGW(TAG, "Display not started");
        return true;
    }

    deinitialize();
    isStarted = false;
    ESP_LOGI(TAG, "Display stopped successfully");
    return true;
}

std::shared_ptr<tt::hal::touch::TouchDevice> YellowDisplay::createTouch() {
    return createYellowTouch();
}

lv_display_t* YellowDisplay::getLvglDisplay() const {
    return lvglDisplay;
}

void YellowDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isStarted) {
        ESP_LOGE(TAG, "setBacklightDuty: Display not started");
        return;
    }

    if (!driver::pwmbacklight::setBacklightDuty(backlightDuty)) {
        ESP_LOGE(TAG, "Failed to set backlight duty to %u", backlightDuty);
    } else {
        ESP_LOGI(TAG, "Backlight duty set to %u", backlightDuty);
    }
}

void YellowDisplay::setRotation(lv_display_rotation_t rotation) {
    if (!panelHandle || !lvglDisplay) {
        ESP_LOGE(TAG, "setRotation: Display not initialized");
        return;
    }

    ESP_LOGI(TAG, "setRotation: Entering, rotation=%d", rotation);
    bool swapXY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorX = (rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90);
    ESP_LOGI(TAG, "setRotation: swapXY=%d, mirrorX=%d, mirrorY=%d", swapXY, mirrorX, mirrorY);

    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panelHandle, swapXY));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panelHandle, mirrorX, mirrorY));
    lv_display_set_rotation(lvglDisplay, rotation);

    ESP_LOGI(TAG, "setRotation: Exiting");
}

void YellowDisplay::initialize() {
    esp_err_t ret;

    // Step 1: Configure and initialize the I80 bus
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,                        // Clock source
        .dc_gpio_num = config->dcPin,                          // DC line GPIO
        .wr_gpio_num = config->wrPin,                          // WR line GPIO
        .data_gpio_nums = {                                    // Data line GPIOs (16 elements)
            config->dataPins[0], config->dataPins[1], config->dataPins[2], config->dataPins[3],
            config->dataPins[4], config->dataPins[5], config->dataPins[6], config->dataPins[7],
            -1, -1, -1, -1, -1, -1, -1, -1                    // Unused pins set to -1
        },
        .bus_width = CYD_2432S022C_LCD_BUS_WIDTH,              // Bus width (e.g., 8)
        .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t),
        .dma_burst_size = 64,                                  // DMA burst size (e.g., 64 bytes)
    };
    esp_lcd_i80_bus_handle_t i80_bus = nullptr;
    ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize i80 bus: %s", esp_err_to_name(ret));
        return;
    }

    // Step 2: Configure and initialize the panel IO
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = config->csPin,                          // CS line GPIO
        .pclk_hz = static_cast<uint32_t>(config->pclkHz),      // Pixel clock frequency
        .trans_queue_depth = 10,                               // Transaction queue depth
        .dc_levels = {                                         // DC signal levels
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1
        },
        .on_color_trans_done = nullptr,                        // No callback
        .user_ctx = nullptr,                                   // No user context
        .lcd_cmd_bits = 8,                                     // 8-bit commands
        .lcd_param_bits = 8,                                   // 8-bit parameters
        .flags = {                                             // Fully initialize flags
            .cs_active_high = 0,
            .reverse_color_bits = 0,
            .swap_color_bytes = 0,
            .pclk_active_neg = 0,
            .pclk_idle_low = 0
        }
    };
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    ret = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize panel IO: %s", esp_err_to_name(ret));
        esp_lcd_del_i80_bus(i80_bus);  // Clean up bus on failure
        return;
    }

    // Step 3: Initialize the LCD panel (e.g., ST7789)
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rstPin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,                                  // RGB565
        .data_endian = LCD_DATA_ENDIAN_BIG,                    // Assuming big-endian data
        .flags = { .reset_active_high = 0 },                   // Reset is active low
        .vendor_config = nullptr
    };
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panelHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ST7789 panel: %s", esp_err_to_name(ret));
        esp_lcd_del_i80_bus(i80_bus);  // Clean up bus on failure
        return;
    }

    // Step 4: Configure and enable the panel
    if (config->rstPin != GPIO_NUM_NC) {
        esp_lcd_panel_reset(panelHandle);
    }
    esp_lcd_panel_init(panelHandle);
    esp_lcd_panel_swap_xy(panelHandle, config->swapXY);
    esp_lcd_panel_mirror(panelHandle, config->mirrorX, config->mirrorY);
    esp_lcd_panel_disp_on_off(panelHandle, true);

    // Step 5: Integrate with LVGL
    lvglDisplay = lv_display_create(config->horizontalResolution, config->verticalResolution);
    if (!lvglDisplay) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        esp_lcd_panel_del(panelHandle);
        panelHandle = nullptr;
        esp_lcd_del_i80_bus(i80_bus);
        return;
    }

    ESP_LOGI(TAG, "YellowDisplay initialized successfully");
}

void YellowDisplay::deinitialize() {
    if (lvglDisplay) {
        lv_display_delete(lvglDisplay);
        lvglDisplay = nullptr;
    }
    if (panelHandle) {
        esp_lcd_panel_del(panelHandle);
        panelHandle = nullptr;
    }
}

std::shared_ptr<DisplayDevice> createDisplay() {
    auto touch = createYellowTouch();

    auto configuration = std::make_unique<YellowDisplay::Configuration>(
        YellowDisplay::Configuration{
            .pclkHz = CYD_2432S022C_LCD_PCLK_HZ,
            .csPin = CYD_2432S022C_LCD_PIN_CS,
            .dcPin = CYD_2432S022C_LCD_PIN_DC,
            .wrPin = CYD_2432S022C_LCD_PIN_WR,
            .rstPin = CYD_2432S022C_LCD_PIN_RST,
            .backlightPin = CYD_2432S022C_LCD_PIN_BACKLIGHT,
            .dataPins = {
                CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1,
                CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
                CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5,
                CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7,
            },
            .horizontalResolution = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
            .verticalResolution = CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
            .touch = touch
        }
    );

    configuration->swapXY = false;
    configuration->mirrorX = false;
    configuration->mirrorY = false;

    return std::make_shared<YellowDisplay>(std::move(configuration));
}

} // namespace tt::hal::display
