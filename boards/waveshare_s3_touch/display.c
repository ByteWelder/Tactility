#include "display_defines_i.h"

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl_i.h"
#include "lvgl.h"
#include <esp_lcd_panel_rgb.h>
#include <esp_timer.h>
#include <sys/cdefs.h>
#include <thread.h>

#define TAG "waveshare_s3_touch_display"

SemaphoreHandle_t sem_vsync_end = NULL;
SemaphoreHandle_t sem_gui_ready = NULL;

SemaphoreHandle_t lvgl_mux = NULL;

#define WAVESHARE_LCD_PIXEL_CLOCK_HZ (12 * 1000 * 1000) // NOTE: original was 14MHz, but we had to slow it down with PSRAM frame buffer
#define WAVESHARE_PIN_NUM_HSYNC 46
#define WAVESHARE_PIN_NUM_VSYNC 3
#define WAVESHARE_PIN_NUM_DE 5
#define WAVESHARE_PIN_NUM_PCLK 7
#define WAVESHARE_PIN_NUM_DATA0 14  // B3
#define WAVESHARE_PIN_NUM_DATA1 38  // B4
#define WAVESHARE_PIN_NUM_DATA2 18  // B5
#define WAVESHARE_PIN_NUM_DATA3 17  // B6
#define WAVESHARE_PIN_NUM_DATA4 10  // B7
#define WAVESHARE_PIN_NUM_DATA5 39  // G2
#define WAVESHARE_PIN_NUM_DATA6 0   // G3
#define WAVESHARE_PIN_NUM_DATA7 45  // G4
#define WAVESHARE_PIN_NUM_DATA8 48  // G5
#define WAVESHARE_PIN_NUM_DATA9 47  // G6
#define WAVESHARE_PIN_NUM_DATA10 21 // G7
#define WAVESHARE_PIN_NUM_DATA11 1  // R3
#define WAVESHARE_PIN_NUM_DATA12 2  // R4
#define WAVESHARE_PIN_NUM_DATA13 42 // R5
#define WAVESHARE_PIN_NUM_DATA14 41 // R6
#define WAVESHARE_PIN_NUM_DATA15 40 // R7
#define WAVESHARE_PIN_NUM_DISP_EN (-1)

#define WAVESHARE_BUFFER_HEIGHT (WAVESHARE_LCD_VER_RES / 3) // How many rows of pixels to buffer - 1/3rd is about 1MB
#define WAVESHARE_LVGL_TICK_PERIOD_MS 2 // TODO: Setting it to 5 causes a crash - why?

#define WAVESHARE_USE_DOUBLE_FB true // Performance boost at the cost of about extra PSRAM(SPIRAM)

#if WAVESHARE_USE_DOUBLE_FB
#define WAVESHARE_LCD_NUM_FB             2
#else
#define WAVESHARE_LCD_NUM_FB             1
#endif // WAVESHARE_USE_DOUBLE_FB

static bool lvgl_is_running = false;
#define LVGL_MAX_SLEEP 500

bool ws3t_display_lock(uint32_t timeout_ms) {
    assert(lvgl_mux && "lvgl_port_init must be called first");
    const TickType_t timeout_ticks = (timeout_ms == 0) ? TtWaitForever : pdMS_TO_TICKS(timeout_ms);
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
    lv_tick_inc(WAVESHARE_LVGL_TICK_PERIOD_MS);
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

lv_disp_t* ws3t_display_create() {
    static lv_disp_drv_t display_driver;
    static lv_disp_draw_buf_t display_buffer;

    ESP_LOGI(TAG, "Create semaphores");
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    Thread* thread = tt_thread_alloc_ex("display_task", 8192, &display_task, NULL);
    tt_thread_set_priority(thread, ThreadPriorityHigh);
    tt_thread_start(thread);

    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = WAVESHARE_LCD_PIXEL_CLOCK_HZ,
            .h_res = WAVESHARE_LCD_HOR_RES,
            .v_res = WAVESHARE_LCD_VER_RES,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 10,
            .hsync_front_porch = 20,
            .hsync_pulse_width = 10,
            .vsync_back_porch = 10,
            .vsync_front_porch = 10,
            .vsync_pulse_width = 10,
        },
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .bits_per_pixel = 16,
        .num_fbs = WAVESHARE_LCD_NUM_FB,
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 0,
        .psram_trans_align = 64,
        .hsync_gpio_num = WAVESHARE_PIN_NUM_HSYNC,
        .vsync_gpio_num = WAVESHARE_PIN_NUM_VSYNC,
        .de_gpio_num = WAVESHARE_PIN_NUM_DE,
        .pclk_gpio_num = WAVESHARE_PIN_NUM_PCLK,
        .disp_gpio_num = WAVESHARE_PIN_NUM_DISP_EN,
        .data_gpio_nums = {
            WAVESHARE_PIN_NUM_DATA0,
            WAVESHARE_PIN_NUM_DATA1,
            WAVESHARE_PIN_NUM_DATA2,
            WAVESHARE_PIN_NUM_DATA3,
            WAVESHARE_PIN_NUM_DATA4,
            WAVESHARE_PIN_NUM_DATA5,
            WAVESHARE_PIN_NUM_DATA6,
            WAVESHARE_PIN_NUM_DATA7,
            WAVESHARE_PIN_NUM_DATA8,
            WAVESHARE_PIN_NUM_DATA9,
            WAVESHARE_PIN_NUM_DATA10,
            WAVESHARE_PIN_NUM_DATA11,
            WAVESHARE_PIN_NUM_DATA12,
            WAVESHARE_PIN_NUM_DATA13,
            WAVESHARE_PIN_NUM_DATA14,
            WAVESHARE_PIN_NUM_DATA15
        },
        .flags = {
            .disp_active_low = false,
            .refresh_on_demand = false,
            .fb_in_psram = true,
#if WAVESHARE_USE_DOUBLE_FB
                  .double_fb = true,
#else
            .double_fb = false,
#endif
            .no_fb = false,
            .bb_invalidate_cache = false
        }
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

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    void *buf1 = NULL;
    void *buf2 = NULL;
#if WAVESHARE_USE_DOUBLE_FB
    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
    buf1 = heap_caps_malloc(WAVESHARE_LCD_HOR_RES * WAVESHARE_BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = heap_caps_malloc(WAVESHARE_LCD_HOR_RES * WAVESHARE_BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&display_buffer, buf1, buf2, WAVESHARE_LCD_HOR_RES * WAVESHARE_BUFFER_HEIGHT);
#else
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    buf1 = heap_caps_malloc(WAVESHARE_LCD_H_RES * WAVESHARE_BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_disp_draw_buf_init(&display_buffer, buf1, buf2, WAVESHARE_LCD_H_RES * WAVESHARE_BUFFER_HEIGHT);
#endif // WAVESHARE_USE_DOUBLE_FB

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = WAVESHARE_LCD_HOR_RES;
    display_driver.ver_res = WAVESHARE_LCD_VER_RES;
    display_driver.flush_cb = display_flush_callback;
    display_driver.draw_buf = &display_buffer;
    display_driver.user_data = panel_handle;
    display_driver.antialiasing = false;
    display_driver.direct_mode = false;
    display_driver.sw_rotate = false;
    display_driver.rotated = 0;
    display_driver.screen_transp = false;

#if WAVESHARE_USE_DOUBLE_FB
    display_driver.full_refresh = true; // Maintains the synchronization between the two frame buffers
#else
    display_driver.full_refresh = false;
#endif

    lv_disp_t* display = lv_disp_drv_register(&display_driver);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, WAVESHARE_LVGL_TICK_PERIOD_MS * 1000));

    return display;
}

void ws3t_display_destroy() {
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
