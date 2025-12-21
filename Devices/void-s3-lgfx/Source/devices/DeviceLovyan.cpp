#include "DeviceLovyan.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LovyanDevice";
static LGFX lcd;
static lv_display_t *disp = nullptr;
static lv_indev_t *indev = nullptr;
static uint8_t *draw_buf = nullptr;

LGFX &getLovyan() {
    return lcd;
}

static void my_disp_flush(lv_display_t *drv, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixelsDMA((uint16_t *)px_map, w * h, true);
    lcd.endWrite();
    lv_display_flush_ready(drv);
}

static void my_touch_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX = 0, touchY = 0;
    if (lcd.getTouch(&touchX, &touchY)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchX;
        data->point.y = touchY;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool lovyan_hw_init() {
    ESP_LOGI(TAG, "Initializing LovyanGFX hardware");
    lcd.init();
    lcd.initDMA();
    lcd.setRotation(1);
    // Default orientation/brightness set by app if desired
    return true;
}

bool lovyan_lvgl_bind(void) {
    if (disp != nullptr) {
        ESP_LOGW(TAG, "LVGL already bound to Lovyan");
        return false;
    }

    int32_t hor_res = lcd.width();
    int32_t ver_res = lcd.height();
    const size_t DRAW_BUF_SIZE = (hor_res * ver_res / 10 * sizeof(lv_color_t));

    draw_buf = (uint8_t *)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA);
    if (!draw_buf) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        return false;
    }

    disp = lv_display_create(hor_res, ver_res);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(draw_buf);
        draw_buf = nullptr;
        return false;
    }

    lv_display_set_physical_resolution(disp, hor_res, ver_res);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_NATIVE);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    indev = lv_indev_create();
    if (!indev) {
        ESP_LOGE(TAG, "Failed to create LVGL input device");
        lv_display_delete(disp);
        disp = nullptr;
        heap_caps_free(draw_buf);
        draw_buf = nullptr;
        return false;
    }
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);

    ESP_LOGI(TAG, "Bound LVGL display and touch to LovyanGFX");
    return true;
}

/* Accessors */

lv_display_t* lovyan_get_display(void) {
    return disp;
}

lv_indev_t* lovyan_get_indev(void) {
    return indev;
}

bool lovyan_lvgl_unbind(void) {
    if (indev) {
        lv_indev_delete(indev);
        indev = nullptr;
    }

    if (disp) {
        lv_display_delete(disp);
        disp = nullptr;
    }

    if (draw_buf) {
        heap_caps_free(draw_buf);
        draw_buf = nullptr;
    }

    ESP_LOGI(TAG, "Unbound LVGL from LovyanGFX");
    return true;
}

void lovyan_set_backlight(uint8_t duty) {
    lcd.setBrightness(duty);
}
