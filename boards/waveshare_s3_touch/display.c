#include "waveshare_s3_touch_defines.h"

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include "ui/lvgl_sync.h"
#include <esp_lcd_panel_rgb.h>
#include <esp_timer.h>
#include <sys/cdefs.h>
#include <thread.h>

#define TAG "waveshare_s3_touch_display"

SemaphoreHandle_t sem_vsync_end = NULL;
SemaphoreHandle_t sem_gui_ready = NULL;

SemaphoreHandle_t lvgl_mux = NULL;

void touch_init(lv_disp_t* display);

static bool display_lock(uint32_t timeout_ms) {
    assert(lvgl_mux && "lvgl_port_init must be called first");
    const TickType_t timeout_ticks = (timeout_ms == 0) ? TtWaitForever : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void display_unlock(void) {
    assert(lvgl_mux && "lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(lvgl_mux);
}

static bool lvgl_is_running = false;
#define LVGL_MAX_SLEEP 500
// Display_task should have lower priority than lvgl_tick_task below
static int32_t display_task(TT_UNUSED void* parameter) {
    uint32_t task_delay_ms = LVGL_MAX_SLEEP;

    ESP_LOGI(TAG, "Starting LVGL task");
    lvgl_is_running = true;
    while (lvgl_is_running) {
        if (display_lock(0)) {
            task_delay_ms = lv_timer_handler();
            display_unlock();
        }
        if ((task_delay_ms > LVGL_MAX_SLEEP) || (1 == task_delay_ms)) {
            task_delay_ms = LVGL_MAX_SLEEP;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    /* Close task */
    vTaskDelete(NULL);
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
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void display_flush_callback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

bool waveshare_s3_touch_create_display() {
    static lv_disp_drv_t display_driver;
    static lv_disp_draw_buf_t display_buffer;

    ESP_LOGI(TAG, "Create semaphores");
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    // Timer task
    Thread* thread = tt_thread_alloc_ex("display_task", 8192, &display_task, NULL);
    tt_thread_set_priority(thread, ThreadPriorityHigh);
    tt_thread_start(thread);

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .sram_trans_align = 0,
        .psram_trans_align = 64,
        .num_fbs = 1,
        .bounce_buffer_size_px = 0,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 30,
            .hsync_front_porch = 210,
            .hsync_pulse_width = 30,
            .vsync_back_porch = 4,
            .vsync_front_porch = 4,
            .vsync_pulse_width = 4,
            .flags.pclk_active_neg = true,
        },
        .flags = {.disp_active_low = false, .refresh_on_demand = false,
                  .fb_in_psram = true, // allocate frame buffer in PSRAM
                  .double_fb = false,
                  .no_fb = false,
                  .bb_invalidate_cache = false}
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = on_vsync_event,
        .on_bounce_empty = NULL,
        .on_bounce_frame_finish = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &display_driver));

    ESP_LOGI(TAG, "Initialize LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    void* display_buffer_bytes = heap_caps_malloc(EXAMPLE_LCD_H_RES * BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA); // TODO: use MALLOC_CAP_DMA for other drivers too
    assert(display_buffer_bytes);
    lv_disp_draw_buf_init(&display_buffer, display_buffer_bytes, NULL, EXAMPLE_LCD_H_RES * BUFFER_HEIGHT);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = EXAMPLE_LCD_H_RES;
    display_driver.ver_res = EXAMPLE_LCD_V_RES;
    display_driver.flush_cb = display_flush_callback;
    display_driver.draw_buf = &display_buffer;
    display_driver.user_data = panel_handle;

#if CONFIG_EXAMPLE_DOUBLE_FB
    disp_drv.full_refresh = true; // the full_refresh mode can maintain the synchronization between the two frame buffers
#endif
    lv_disp_t* disp = lv_disp_drv_register(&display_driver);

    touch_init(disp);

    // Tick task
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    return true;
}

bool waveshare_s3_touch_init_lvgl() {
    tt_lvgl_sync_set(&display_lock, &display_unlock);
    return waveshare_s3_touch_create_display();
}

void waveshare_s3_touch_deinit_lvgl() {
    // TODO: de-init display, its buffer and touch, stop display tasks, stop timer
    // TODO: see esp_lvlg_port.c for more info
    if (lvgl_mux) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = NULL;
    }
#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}
