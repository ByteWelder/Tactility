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
#include "esp_heap_caps.h"  // For MALLOC_CAP_DMA

#define TAG "YellowDisplay"

namespace tt::hal::display {

YellowDisplay::YellowDisplay(std::unique_ptr<Configuration> config)
    : config(std::move(config)), panelHandle(nullptr), lvglDisplay(nullptr), isStarted(false), drawBuffer(nullptr) {
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

esp_lcd_panel_handle_t YellowDisplay::getPanelHandle() const {
    return panelHandle;
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
        .dc_gpio_num = config->dcPin,
        .wr_gpio_num = config->wrPin,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            config->dataPins[0], config->dataPins[1], config->dataPins[2], config->dataPins[3],
            config->dataPins[4], config->dataPins[5], config->dataPins[6], config->dataPins[7],
            -1, -1, -1, -1, -1, -1, -1, -1  // Fill remaining pins with -1
        },
        .bus_width = CYD_2432S022C_LCD_BUS_WIDTH,
        .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t),
        .dma_burst_size = 64,
        .sram_trans_align = 0  // Deprecated field, set to 0
    };
    esp_lcd_i80_bus_handle_t i80_bus = nullptr;
    ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize i80 bus: %s", esp_err_to_name(ret));
        return;
    }

    // Step 2: Configure and initialize the panel IO
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = config->csPin,
        .pclk_hz = static_cast<uint32_t>(config->pclkHz),
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1
        },
        .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
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
        esp_lcd_del_i80_bus(i80_bus);
        return;
    }

    // Step 3: Initialize the LCD panel (e.g., ST7789)
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rstPin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,  // Corrected from LCD_DATA_ENDIAN_BIG
        .flags = { .reset_active_high = 0 },
        .vendor_config = nullptr
    };
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panelHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ST7789 panel: %s", esp_err_to_name(ret));
        esp_lcd_del_i80_bus(i80_bus);
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

    // Step 5: Allocate DMA-optimized draw buffer
    size_t buffer_size = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(lv_color_t);  // 7680 pixels * 2 bytes
    drawBuffer = esp_lcd_i80_alloc_draw_buffer(io_handle, buffer_size, MALLOC_CAP_DMA);
    if (!drawBuffer) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        esp_lcd_panel_del(panelHandle);
        panelHandle = nullptr;
        esp_lcd_del_i80_bus(i80_bus);
        return;
    }

    // Step 6: Create LVGL display
    lvglDisplay = lv_display_create(CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
    if (!lvglDisplay) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(drawBuffer);
        drawBuffer = nullptr;
        esp_lcd_panel_del(panelHandle);
        panelHandle = nullptr;
        esp_lcd_del_i80_bus(i80_bus);
        return;
    }

    // Set the draw buffer and render mode to partial
    lv_display_set_draw_buffers(lvglDisplay, drawBuffer, nullptr, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set the flush callback
    lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
        auto* yellowDisp = static_cast<YellowDisplay*>(lv_display_get_user_data(disp));
        esp_lcd_panel_draw_bitmap(yellowDisp->getPanelHandle(), area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
        lv_display_flush_ready(disp);
    });

    // Set user data for the flush callback
    lv_display_set_user_data(lvglDisplay, this);

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
    if (drawBuffer) {
        heap_caps_free(drawBuffer);  // Free the DMA-allocated buffer
        drawBuffer = nullptr;
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
