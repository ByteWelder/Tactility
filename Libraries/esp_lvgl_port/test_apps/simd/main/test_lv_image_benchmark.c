/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <malloc.h>
#include <sdkconfig.h>

#include "unity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // for xthal_get_ccount()
#include "lv_image_common.h"
#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_rgb565.h"

#define COMMON_DIM 128      // Common matrix dimension 128x128 pixels
#define WIDTH COMMON_DIM
#define HEIGHT COMMON_DIM
#define STRIDE WIDTH
#define UNALIGN_BYTES 3
#define BENCHMARK_CYCLES 1000

// ------------------------------------------------ Static variables ---------------------------------------------------

static const char *TAG_LV_IMAGE_BENCH = "LV Image Benchmark";
static const char *asm_ansi_func[] = {"ASM", "ANSI"};

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Initialize the benchmark test
 */
static void lv_image_benchmark_init(bench_test_case_lv_image_params_t *test_params);

/**
 * @brief Run the benchmark test
 */
static float lv_image_benchmark_run(bench_test_case_lv_image_params_t *test_params, _lv_draw_sw_blend_image_dsc_t *dsc);

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
    - Firstly use an input test parameters for the most ideal case (16-byte aligned arrays, arrays widths divisible by 2 for RGB565 color format)
    - Then use worst-case input test parameters (1-byte aligned arrays, arrays width NOT divisible by 2 for RGB565 color format)
    - Count how many CPU cycles does it take to run a function from the LVGL blending API for each case (ideal and worst case)
    - Run ansi version of LVGL blending API multiple times (1000-times or so) and repeat the 2 above steps for the ansi version
    - Compare the results
    - Free test arrays and structures needed for LVGL blending API

Inducing Most ideal and worst case scenarios:
    - Most ideal:
        - Both, the source and the destination buffers should be aligned by 16-byte (Xtensa PIE), or 4-byte (Xtensa base) boundaries
        - Matrix width (in pixels) should be equal to the main loop length in the assembly src code
          typically multiples of 16 bytes (for RGB565 it's either 32 bytes - 16 pixels or 48 bytes - 24 pixels)
        - Matrix height does not have any effect on benchmark unit tests, unit the matrix is too large that cache limitations start to affect the performance
        - Matrix strides, should be equal to the matrix widths (0 matrix padding), or their multiples (matrix width = matrix padding)
    - Worst case:
        - Both, hte source and the destination buffers should NOT be aligned by 16-byte (Xtensa PIE), or 4-byte (Xtensa base) boundaries,
          Source buffer unalignment should be different from the destination unalignment, with one unalignment being even, the other being odd
          The unalignments shall be small numbers (preferably 1 or 2 bytes)
        - Matrix width should be one pixels smaller, than the matrix width for the most ideal case
        - Matrix height does not have any effect on benchmark unit tests, unit the matrix is too large that cache limitations start to affect the performance
        - Matrix strides, should NOT be equal to the matrix widths (non 0 matrix padding)
*/
// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("LV Image benchmark RGB565 blend to RGB565", "[image][benchmark][RGB565]")
{
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + UNALIGN_BYTES);
    uint16_t *src_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + UNALIGN_BYTES);
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);
    TEST_ASSERT_NOT_EQUAL(NULL, src_array_align16);

    // Apply byte unalignment (different for each array) for the worst-case test scenario
    uint16_t *dest_array_align1 = (uint16_t *)((uint8_t *)dest_array_align16 + UNALIGN_BYTES - 1);
    uint16_t *src_array_align1 = (uint16_t *)((uint8_t *)src_array_align16 + UNALIGN_BYTES);

    bench_test_case_lv_image_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .dest_stride = STRIDE * sizeof(uint16_t),
        .src_stride = STRIDE * sizeof(uint16_t),
        .cc_height = HEIGHT,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .src_array_align16 = (void *)src_array_align16,
        .src_array_align1 = (void *)src_array_align1,
        .dest_array_align16 = (void *)dest_array_align16,
        .dest_array_align1 = (void *)dest_array_align1,
        .blend_api_func = &lv_draw_sw_blend_image_to_rgb565,
    };

    ESP_LOGI(TAG_LV_IMAGE_BENCH, "running test for RGB565 color format");
    lv_image_benchmark_init(&test_params);
    free(dest_array_align16);
    free(src_array_align16);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_image_benchmark_init(bench_test_case_lv_image_params_t *test_params)
{
    // Init structure for LVGL blend API, to call the Assembly API
    _lv_draw_sw_blend_image_dsc_t dsc = {
        .dest_buf = test_params->dest_array_align16,
        .dest_w = test_params->width,
        .dest_h = test_params->height,
        .dest_stride = test_params->dest_stride,  // stride * sizeof()
        .mask_buf = NULL,
        .src_buf = test_params->src_array_align16,
        .src_stride = test_params->src_stride,
        .src_color_format = LV_COLOR_FORMAT_RGB565,
        .opa = LV_OPA_MAX,
        .blend_mode = LV_BLEND_MODE_NORMAL,
        .use_asm = true,
    };

    // Init structure for LVGL blend API, to call the ANSI API
    _lv_draw_sw_blend_image_dsc_t dsc_cc = dsc;
    dsc_cc.dest_buf = test_params->dest_array_align1;
    dsc_cc.dest_w = test_params->cc_width;
    dsc_cc.dest_h = test_params->cc_height;
    dsc_cc.src_buf = test_params->src_array_align1;

    // Run benchmark 2 times:
    // First run using assembly, second run using ANSI
    for (int i = 0; i < 2; i++) {

        // Run benchmark with the most ideal input parameters
        float cycles = lv_image_benchmark_run(test_params, &dsc);        // Call Benchmark cycle
        float per_sample = cycles / ((float)(dsc.dest_w * dsc.dest_h));
        ESP_LOGI(TAG_LV_IMAGE_BENCH, " %s ideal case: %.3f cycles for %"PRIi32"x%"PRIi32" matrix, %.3f cycles per sample", asm_ansi_func[i], cycles, dsc.dest_w, dsc.dest_h, per_sample);

        // Run benchmark with the corner case input parameters
        cycles = lv_image_benchmark_run(test_params, &dsc_cc);           // Call Benchmark cycle
        per_sample = cycles / ((float)(dsc_cc.dest_w * dsc_cc.dest_h));
        ESP_LOGI(TAG_LV_IMAGE_BENCH, " %s corner case: %.3f cycles for %"PRIi32"x%"PRIi32" matrix, %.3f cycles per sample\n", asm_ansi_func[i], cycles, dsc_cc.dest_w, dsc_cc.dest_h, per_sample);

        // change to ANSI
        dsc.use_asm = false;
        dsc_cc.use_asm = false;
    }
}

static float lv_image_benchmark_run(bench_test_case_lv_image_params_t *test_params, _lv_draw_sw_blend_image_dsc_t *dsc)
{
    // Call the DUT function for the first time to init the benchmark test
    test_params->blend_api_func(dsc);

    const unsigned int start_b = xthal_get_ccount();
    for (int i = 0; i < test_params->benchmark_cycles; i++) {
        test_params->blend_api_func(dsc);
    }
    const unsigned int end_b = xthal_get_ccount();

    const float total_b = end_b - start_b;
    const float cycles = total_b / (test_params->benchmark_cycles);
    return cycles;
}
