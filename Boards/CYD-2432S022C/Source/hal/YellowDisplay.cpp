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

#define TAG "YellowDisplay"

namespace tt::hal::display {

YellowDisplay::YellowDisplay(std::unique_ptr<Configuration> config)
    : config(std::move(config)), panelHandle(nullptr), lvglDisplay(nullptr), isStarted(false) {
    // Initialization deferred to start()
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

    isStarted = true;
    ESP_LOGI(TAG, "Display started successfully");

    // Apply initial backlight duty from preferences
    setBacklightDuty(tt::app::display::getBacklightDuty());

    // Apply initial rotation from preferences
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
    return createYellowTouch();  // Delegates to YellowTouch.cpp
}

lv_display_t* YellowDisplay::getLvglDisplay() const {
    return lvglDisplay;
}

void YellowDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isStarted || !panelHandle) {
        ESP_LOGE(TAG, "setBacklightDuty: Display not initialized");
        return;
    }

    // Map 0-255 to GPIO level (0 = off, 255 = on)
    bool level = (backlightDuty > 127) ? CYD_2432S022C_LCD_BACKLIGHT_ON_LEVEL : !CYD_2432S022C_LCD_BACKLIGHT_ON_LEVEL;
    gpio_set_level(config->backlightPin, level);
    ESP_LOGI(TAG, "Backlight duty set to %u (GPIO level: %d)", backlightDuty, level);
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

    // Initialize i80 bus
    esp_lcd_i80_bus_handle_t i80_bus = nullptr;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = config->dcPin,
        .wr_gpio_num = config->wrPin,
        .data_gpio_nums = {
            config->dataPins[0], config->dataPins[1], config->dataPins[2], config->dataPins[3],
            config->dataPins[4], config->dataPins[5], config->dataPins[6], config->dataPins[7],
        },
        .bus_width = CYD_2432S022C_LCD_BUS_WIDTH,
        .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t),
    };
    ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize i80 bus: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize panel IO
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = config->csPin,
        .pclk_hz = config->pclkHz,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .cs_active_high = 0,  // CS is active low
        },
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ret = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize panel IO: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize ST7789 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rstPin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,  // RGB565
    };
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panelHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ST7789 panel: %s", esp_err_to_name(ret));
        return;
    }

    // Reset and initialize the panel
    if (config->rstPin != GPIO_NUM_NC) {
        esp_lcd_panel_reset(panelHandle);
    }
    esp_lcd_panel_init(panelHandle);

    // Apply initial orientation from config
    esp_lcd_panel_swap_xy(panelHandle, config->swapXY);
    esp_lcd_panel_mirror(panelHandle, config->mirrorX, config->mirrorY);

    // Turn on display
    esp_lcd_panel_disp_on_off(panelHandle, true);

    // Initialize backlight GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << config->backlightPin,
    };
    gpio_config(&bk_gpio_config);

    // Create LVGL display
    lvglDisplay = lv_display_create(config->horizontalResolution, config->verticalResolution);
    if (!lvglDisplay) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        esp_lcd_panel_del(panelHandle);
        panelHandle = nullptr;
        return;
    }

    // Store this object as user data for flush callback
    lvglDisplay->user_data = this;

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

    // Set initial orientation (will be overridden by start())
    configuration->swapXY = false;
    configuration->mirrorX = false;
    configuration->mirrorY = false;

    return std::make_shared<YellowDisplay>(std::move(configuration));
}

} // namespace tt::hal::display
