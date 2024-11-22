#include "config.h"

#include "lvgl.h"
#include "TactilityCore.h"
#include "thread.h"

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include <esp_lcd_panel_rgb.h>
#include <esp_timer.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define TAG "waveshare_s3_touch_display"

SemaphoreHandle_t sem_vsync_end = nullptr;
SemaphoreHandle_t sem_gui_ready = nullptr;
SemaphoreHandle_t lvgl_mux = nullptr;

#if WAVESHARE_LCD_USE_DOUBLE_FB
#define WAVESHARE_LCD_NUM_FB 2
#else
#define WAVESHARE_LCD_NUM_FB 1
#endif

static bool lvgl_is_running = false;

bool ws3t_display_lock(uint32_t timeout_ms) {
    assert(lvgl_mux && "lvgl_port_init must be called first");
    const TickType_t timeout_ticks = (timeout_ms == 0) ? tt::TtWaitForever : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void ws3t_display_unlock(void) {
    assert(lvgl_mux && "lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(lvgl_mux);
}

// Display_task should have lower priority than lvgl_tick_task below
static int32_t display_task(TT_UNUSED void* parameter) {
    uint32_t task_delay_ms = LVGL_MAX_SLEEP;

    ESP_LOGI(TAG, "Starting LVGL task");
    lvgl_is_running = true;
    while (lvgl_is_running) {
        if (ws3t_display_lock(0)) {
            task_delay_ms = lv_timer_handler();
            ws3t_display_unlock();
        }
        if ((task_delay_ms > LVGL_MAX_SLEEP) || (1 == task_delay_ms)) {
            task_delay_ms = LVGL_MAX_SLEEP;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    vTaskDelete(nullptr);
    return 0;
}

static bool on_vsync_event(
    TT_UNUSED esp_lcd_panel_handle_t panel,
    TT_UNUSED const esp_lcd_rgb_panel_event_data_t* event_data,
    TT_UNUSED void* user_data
) {
    BaseType_t high_task_awoken = pdFALSE;

    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }

    return high_task_awoken == pdTRUE;
}

static void lvgl_tick_task(TT_UNUSED void* arg) {
    // Tell how much time has passed
    lv_tick_inc(WAVESHARE_LVGL_TICK_PERIOD_MS);
}

static void display_flush_callback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto panel_handle = static_cast<esp_lcd_panel_handle_t>(lv_display_get_user_data(disp));
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    lv_disp_flush_ready(disp);
}

lv_disp_t* ws3t_display_create() {
    TT_LOG_I(TAG, "display init");

    sem_vsync_end = xSemaphoreCreateBinary();
    tt_assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    tt_assert(sem_gui_ready);

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    tt_assert(lvgl_mux);

    tt::Thread* thread = tt::thread_alloc_ex("display_task", 8192, &display_task, nullptr);
    tt::thread_set_priority(thread, tt::ThreadPriorityHigh); // TODO: try out THREAD_PRIORITY_RENDER
    tt::thread_start(thread);

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = WAVESHARE_LCD_PIXEL_CLOCK_HZ,
            .h_res = WAVESHARE_LCD_HOR_RES,
            .v_res = WAVESHARE_LCD_VER_RES,
            .hsync_pulse_width = 10,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 10,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 10,
            .vsync_front_porch = 10,
        },
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .bits_per_pixel = 16,
        .num_fbs = WAVESHARE_LCD_NUM_FB,
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 0,
        .psram_trans_align = 64,
        .hsync_gpio_num = WAVESHARE_LCD_PIN_NUM_HSYNC,
        .vsync_gpio_num = WAVESHARE_LCD_PIN_NUM_VSYNC,
        .de_gpio_num = WAVESHARE_LCD_PIN_NUM_DE,
        .pclk_gpio_num = WAVESHARE_LCD_PIN_NUM_PCLK,
        .disp_gpio_num = WAVESHARE_LCD_PIN_NUM_DISP_EN,
        .data_gpio_nums = {WAVESHARE_LCD_PIN_NUM_DATA0, WAVESHARE_LCD_PIN_NUM_DATA1, WAVESHARE_LCD_PIN_NUM_DATA2, WAVESHARE_LCD_PIN_NUM_DATA3, WAVESHARE_LCD_PIN_NUM_DATA4, WAVESHARE_LCD_PIN_NUM_DATA5, WAVESHARE_LCD_PIN_NUM_DATA6, WAVESHARE_LCD_PIN_NUM_DATA7, WAVESHARE_LCD_PIN_NUM_DATA8, WAVESHARE_LCD_PIN_NUM_DATA9, WAVESHARE_LCD_PIN_NUM_DATA10, WAVESHARE_LCD_PIN_NUM_DATA11, WAVESHARE_LCD_PIN_NUM_DATA12, WAVESHARE_LCD_PIN_NUM_DATA13, WAVESHARE_LCD_PIN_NUM_DATA14, WAVESHARE_LCD_PIN_NUM_DATA15},
        .flags = {.disp_active_low = false, .refresh_on_demand = false, .fb_in_psram = true, .double_fb = WAVESHARE_LCD_USE_DOUBLE_FB, .no_fb = false, .bb_invalidate_cache = false}
    };

    if (esp_lcd_new_rgb_panel(&panel_config, &panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create RGB panel");
        return nullptr;
    }

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = on_vsync_event,
        .on_bounce_empty = nullptr,
        .on_bounce_frame_finish = nullptr
    };
    if (esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, nullptr) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to register callbacks");
        return nullptr;
    }

    if (esp_lcd_panel_reset(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return nullptr;
    }

    if (esp_lcd_panel_init(panel_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return nullptr;
    }

    lv_init();

#if WAVESHARE_LCD_USE_DOUBLE_FB
    // initialize LVGL draw buffers
    lv_draw_buf_t* buffer1 = lv_draw_buf_create(WAVESHARE_LCD_HOR_RES, WAVESHARE_LCD_BUFFER_HEIGHT, LV_COLOR_FORMAT_RGB565, 0);
    lv_draw_buf_t* buffer2 = lv_draw_buf_create(WAVESHARE_LCD_HOR_RES, WAVESHARE_LCD_BUFFER_HEIGHT, LV_COLOR_FORMAT_RGB565, 0);
    tt_assert(buffer1);
    tt_assert(buffer2);
#else
    lv_draw_buf_t* buffer1 = lv_draw_buf_create(WAVESHARE_LCD_HOR_RES, WAVESHARE_LCD_VER_RES, LV_COLOR_FORMAT_RGB565, 0);
    lv_draw_buf_t* buffer2 = NULL;
    tt_assert(buffer1);
#endif // WAVESHARE_USE_DOUBLE_FB

    lv_display_t* display = lv_display_create(WAVESHARE_LCD_HOR_RES, WAVESHARE_LCD_VER_RES);
    lv_display_set_draw_buffers(display, buffer1, buffer2);
    lv_display_set_flush_cb(display, &display_flush_callback);
    lv_display_set_user_data(display, panel_handle);
    lv_display_set_antialiasing(display, false);
    lv_display_set_render_mode(display, LV_DISPLAY_RENDER_MODE_PARTIAL);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, WAVESHARE_LVGL_TICK_PERIOD_MS * 1000));

    return display;
}

void ws3t_display_destroy() {
    // TODO: de-init display, its buffer and touch, stop display tasks, stop timer
    // TODO: see esp_lvlg_port.c for more info
    if (lvgl_mux) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = nullptr;
    }
#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}
