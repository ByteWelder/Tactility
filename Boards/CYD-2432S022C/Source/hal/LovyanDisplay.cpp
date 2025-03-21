#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <esp_log.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

static const char* TAG = "LovyanDisplay";

// LGFX_CYD_2432S022C constructor
LovyanGFXDisplay::LGFX_CYD_2432S022C::LGFX_CYD_2432S022C() {
    ESP_LOGI(TAG, "Initializing LGFX_CYD_2432S022C display class");
    
    auto cfg = _bus_instance.config();
    cfg.pin_wr = CYD_2432S022C_LCD_PIN_WR;
    cfg.pin_rd = CYD_2432S022C_LCD_PIN_RD;
    cfg.pin_rs = CYD_2432S022C_LCD_PIN_DC;
    cfg.pin_d0 = CYD_2432S022C_LCD_PIN_D0;
    cfg.pin_d1 = CYD_2432S022C_LCD_PIN_D1;
    cfg.pin_d2 = CYD_2432S022C_LCD_PIN_D2;
    cfg.pin_d3 = CYD_2432S022C_LCD_PIN_D3;
    cfg.pin_d4 = CYD_2432S022C_LCD_PIN_D4;
    cfg.pin_d5 = CYD_2432S022C_LCD_PIN_D5;
    cfg.pin_d6 = CYD_2432S022C_LCD_PIN_D6;
    cfg.pin_d7 = CYD_2432S022C_LCD_PIN_D7;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto pcfg = _panel_instance.config();
    pcfg.pin_cs = CYD_2432S022C_LCD_PIN_CS;
    pcfg.pin_rst = CYD_2432S022C_LCD_PIN_RST;
    pcfg.pin_busy = -1;
    pcfg.panel_width = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;
    pcfg.panel_height = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;
    pcfg.offset_x = 0;
    pcfg.offset_y = 0;
    pcfg.offset_rotation = 0;
    pcfg.dummy_read_pixel = 8;
    pcfg.dummy_read_bits = 1;
    pcfg.readable = true;
    pcfg.invert = false;
    pcfg.rgb_order = false;
    pcfg.dlen_16bit = false;
    pcfg.bus_shared = true;
    _panel_instance.config(pcfg);

    auto lcfg = _light_instance.config();
    lcfg.pin_bl = GPIO_NUM_0;
    lcfg.invert = false;
    lcfg.freq = 44100;
    lcfg.pwm_channel = 7;
    _light_instance.config(lcfg);
    _panel_instance.setLight(&_light_instance);

    setPanel(&_panel_instance);
    ESP_LOGI(TAG, "LGFX_CYD_2432S022C initialization complete");
}

// LovyanGFXDisplay methods
LovyanGFXDisplay::LovyanGFXDisplay(std::unique_ptr<Configuration> config)
    : configuration(std::move(config)) {
    ESP_LOGI(TAG, "LovyanGFXDisplay constructor called with width=%d, height=%d",
             configuration->width, configuration->height);
}

LovyanGFXDisplay::~LovyanGFXDisplay() {
    stop();
}

bool LovyanGFXDisplay::start() {
    if (isStarted) {
        ESP_LOGW(TAG, "Display already started");
        return true;
    }

    ESP_LOGI(TAG, "Starting LovyanGFX display");
    lcd.init();
    lcd.writeCommand(0x11); // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd.writeCommand(0x3A); // Color Mode
    lcd.writeData(0x05);    // RGB565
    lcd.writeCommand(0x29); // Display ON
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd.setBrightness(0);
    lcd.setRotation(0);  // Start with portrait per Init.cpp

    ESP_LOGI(TAG, "Creating LVGL display: %dx%d", configuration->width, configuration->height);
    lvglDisplay = lv_display_create(configuration->width, configuration->height);  // 240x320
    if (!lvglDisplay) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return false;
    }

    lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Heap free before buffer allocation: %d bytes (DMA: %d bytes)",
             heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
             heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    static uint16_t* buf1 = nullptr;
    static uint16_t* buf2 = nullptr;
    buf1 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Failed to allocate buffers! Size requested: %d bytes",
                 CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t));
        return false;
    }
    ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes",
             buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t));

    ESP_LOGI(TAG, "Setting LVGL buffers with render mode PARTIAL");
    lv_display_set_buffers(lvglDisplay, buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    ESP_LOGI(TAG, "LVGL buffers set successfully");

    ESP_LOGI(TAG, "Setting flush callback");
    lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
        auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
        if (!display) {
            ESP_LOGE(TAG, "Flush callback: display is null!");
            lv_display_flush_ready(disp);
            return;
        }
        ESP_LOGI(TAG, "Flush callback: x1=%" PRId32 ", y1=%" PRId32 ", x2=%" PRId32 ", y2=%" PRId32,
                 area->x1, area->y1, area->x2, area->y2);
        display->lcd.startWrite();
        display->lcd.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
        display->lcd.writePixels((lgfx::rgb565_t*)data, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
        display->lcd.endWrite();
        lv_display_flush_ready(disp);
    });

    isStarted = true;
    ESP_LOGI(TAG, "LovyanGFX display started successfully");

    lv_display_rotation_t currentRotation = lv_disp_get_rotation(lvglDisplay);
    setRotation(currentRotation);

    return true;
}

bool LovyanGFXDisplay::stop() {
    if (!isStarted) return true;
    if (configuration && configuration->touch) configuration->touch->stop();
    if (lvglDisplay) lv_display_delete(lvglDisplay);
    lvglDisplay = nullptr;
    isStarted = false;
    return true;
}

void LovyanGFXDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (isStarted) lcd.setBrightness(backlightDuty);
}

bool LovyanGFXDisplay::supportsBacklightDuty() const { return true; }

lv_display_t* LovyanGFXDisplay::getLvglDisplay() const { return lvglDisplay; }

std::shared_ptr<tt::hal::touch::TouchDevice> LovyanGFXDisplay::createTouch() {
    return configuration ? configuration->touch : nullptr;
}

std::string LovyanGFXDisplay::getName() const { return "CYD-2432S022C Display"; }

std::string LovyanGFXDisplay::getDescription() const { return "LovyanGFX-based display for CYD-2432S022C board"; }

void LovyanGFXDisplay::setRotation(lv_display_rotation_t rotation) {
    if (!isStarted) {
        ESP_LOGW(TAG, "Cannot set rotation: display not started");
        return;
    }

    ESP_LOGI(TAG, "Setting rotation to %d", rotation);
    switch (rotation) {
        case LV_DISPLAY_ROTATION_0:   // Portrait (240x320)
            lcd.setRotation(0);
            lv_display_set_resolution(lvglDisplay, configuration->width, configuration->height);  // 240x320
            break;
        case LV_DISPLAY_ROTATION_90:  // Landscape (320x240)
            lcd.setRotation(1);
            lv_display_set_resolution(lvglDisplay, configuration->height, configuration->width);  // 320x240
            break;
        case LV_DISPLAY_ROTATION_180: // Portrait upside-down (240x320)
            lcd.setRotation(2);
            lv_display_set_resolution(lvglDisplay, configuration->width, configuration->height);  // 240x320
            break;
        case LV_DISPLAY_ROTATION_270: // Landscape opposite (320x240)
            lcd.setRotation(3);
            lv_display_set_resolution(lvglDisplay, configuration->height, configuration->width);  // 320x240
            break;
    }

    if (configuration && configuration->touch) {
        static_cast<CST820Touch*>(configuration->touch.get())->setRotation(rotation);
    }
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    auto config = std::make_unique<LovyanGFXDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,  // 240
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,    // 320
        touch
    );
    return std::make_shared<LovyanGFXDisplay>(std::move(config));
}
