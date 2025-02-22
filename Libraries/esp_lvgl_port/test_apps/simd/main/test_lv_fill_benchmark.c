/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <malloc.h>
#include <sdkconfig.h>

#include "unity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // for xthal_get_ccount()
#include "lv_fill_common.h"
#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_argb8888.h"
#include "lv_draw_sw_blend_to_rgb565.h"
#include "lv_draw_sw_blend_to_rgb888.h"

#define WIDTH 128
#define HEIGHT 128
#define STRIDE WIDTH
#define UNALIGN_BYTES 1
#define BENCHMARK_CYCLES 1000

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char *TAG_LV_FILL_BENCH = "LV Fill Benchmark";
static const char *asm_ansi_func[] = {"ASM", "ANSI"};
static lv_color_t test_color = {
    .blue = 0x56,
    .green = 0x34,
    .red = 0x12,
};

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Initialize the benchmark test
 */
static void lv_fill_benchmark_init(bench_test_case_params_t *test_params);

/**
 * @brief Run the benchmark test
 */
static float lv_fill_benchmark_run(bench_test_case_params_t *test_params, _lv_draw_sw_blend_fill_dsc_t *dsc);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Benchmark tests

Requires:
    - To pass functionality tests first

Purpose:
    - Test that an acceleration is achieved by an assembly implementation of LVGL blending API

Procedure:
    - Initialize input parameters (test array length, width, allocate array...) of the benchmark test
    - Run assembly version of LVGL blending API multiple times (1000-times or so)
    - Firstly use an input test parameters for the most ideal case (16-byte aligned array, array width and height divisible by 4 for ARGB8888 color format)
    - Then use worst-case input test parameters (1-byte aligned array, array width and height NOT divisible by 4 for ARGB8888 color format)
    - Count how many CPU cycles does it take to run a function from the LVGL blending API for each case (ideal and worst case)
    - Run ansi version of LVGL blending API multiple times (1000-times or so) and repeat the 2 above steps for the ansi version
    - Free test arrays and structures needed for LVGL blending API
*/
// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("LV Fill benchmark ARGB8888", "[fill][benchmark][ARGB8888]")
{
    uint32_t *dest_array_align16  = (uint32_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint32_t) + UNALIGN_BYTES);
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint32_t *dest_array_align1 = dest_array_align16 + UNALIGN_BYTES;

    bench_test_case_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE * sizeof(uint32_t),
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
        .blend_api_func = &lv_draw_sw_blend_color_to_argb8888,
    };

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for ARGB8888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

TEST_CASE("LV Fill benchmark RGB565", "[fill][benchmark][RGB565]")
{
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + UNALIGN_BYTES);
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint16_t *dest_array_align1 = dest_array_align16 + UNALIGN_BYTES;

    bench_test_case_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE * sizeof(uint16_t),
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
        .blend_api_func = &lv_draw_sw_blend_color_to_rgb565,
    };

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for RGB565 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

TEST_CASE("LV Fill benchmark RGB888", "[fill][benchmark][RGB888]")
{
    uint8_t *dest_array_align16  = (uint8_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint8_t) * 3 + UNALIGN_BYTES);
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint8_t *dest_array_align1 = dest_array_align16 + UNALIGN_BYTES;

    bench_test_case_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE * 3,
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
        .blend_api_px_func = &lv_draw_sw_blend_color_to_rgb888,
    };

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for RGB888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_fill_benchmark_init(bench_test_case_params_t *test_params)
{
    // Init structure for LVGL blend API, to call the Assembly API
    _lv_draw_sw_blend_fill_dsc_t dsc = {
        .dest_buf = test_params->array_align16,
        .dest_w = test_params->width,
        .dest_h = test_params->height,
        .dest_stride = test_params->stride,  // stride * sizeof()
        .mask_buf = NULL,
        .color = test_color,
        .opa = LV_OPA_MAX,
        .use_asm = true,
    };

    // Init structure for LVGL blend API, to call the ANSI API
    _lv_draw_sw_blend_fill_dsc_t dsc_cc = dsc;
    dsc_cc.dest_buf = test_params->array_align1;
    dsc_cc.dest_w = test_params->cc_width;
    dsc_cc.dest_h = test_params->cc_height;

    // Run benchmark 2 times:
    // First run using assembly, second run using ANSI
    for (int i = 0; i < 2; i++) {

        // Run benchmark with the most ideal input parameters
        // Dest array is 16 byte aligned, dest_w and dest_h are dividable by 4
        float cycles = lv_fill_benchmark_run(test_params, &dsc);        // Call Benchmark cycle
        float per_sample = cycles / ((float)(dsc.dest_w * dsc.dest_h));
        ESP_LOGI(TAG_LV_FILL_BENCH, " %s ideal case: %.3f cycles for %"PRIi32"x%"PRIi32" matrix, %.3f cycles per sample", asm_ansi_func[i], cycles, dsc.dest_w, dsc.dest_h, per_sample);

        // Run benchmark with the corner case input parameters
        // Dest array is 1 byte aligned, dest_w and dest_h are not dividable by 4
        cycles = lv_fill_benchmark_run(test_params, &dsc_cc);           // Call Benchmark cycle
        per_sample = cycles / ((float)(dsc_cc.dest_w * dsc_cc.dest_h));
        ESP_LOGI(TAG_LV_FILL_BENCH, " %s corner case: %.3f cycles for %"PRIi32"x%"PRIi32" matrix, %.3f cycles per sample\n", asm_ansi_func[i], cycles, dsc_cc.dest_w, dsc_cc.dest_h, per_sample);

        // change to ANSI
        dsc.use_asm = false;
        dsc_cc.use_asm = false;
    }
}

static float lv_fill_benchmark_run(bench_test_case_params_t *test_params, _lv_draw_sw_blend_fill_dsc_t *dsc)
{
    // Call the DUT function for the first time to init the benchmark test
    if (test_params->blend_api_func != NULL) {
        test_params->blend_api_func(dsc);
    } else if (test_params->blend_api_px_func != NULL) {
        test_params->blend_api_px_func(dsc, 3);
    }

    const unsigned int start_b = xthal_get_ccount();
    if (test_params->blend_api_func != NULL) {
        for (int i = 0; i < test_params->benchmark_cycles; i++) {
            test_params->blend_api_func(dsc);
        }
    } else if (test_params->blend_api_px_func != NULL) {
        for (int i = 0; i < test_params->benchmark_cycles; i++) {
            test_params->blend_api_px_func(dsc, 3);
        }
    }
    const unsigned int end_b = xthal_get_ccount();

    const float total_b = end_b - start_b;
    const float cycles = total_b / (test_params->benchmark_cycles);
    return cycles;
}
