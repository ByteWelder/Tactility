/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "unity.h"
#include "esp_log.h"
#include "lv_image_common.h"
#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_rgb565.h"

// ------------------------------------------------- Defines -----------------------------------------------------------

#define DBG_PRINT_OUTPUT false

// ------------------------------------------------- Macros and Types --------------------------------------------------

#define UPDATE_TEST_CASE(test_case_ptr, dest_w, dest_h, src_stride, dest_stride, src_unalign_byte, dest_unalign_byte) ({  \
    (test_case_ptr)->src_buf_len = (size_t)(dest_h * src_stride);                                 \
    (test_case_ptr)->active_dest_buf_len = (size_t)(dest_h * dest_stride);                        \
    (test_case_ptr)->total_dest_buf_len = (size_t)((dest_h * dest_stride) + (test_case_ptr->canary_pixels * 2));  \
    (test_case_ptr)->dest_w = (dest_w);                         \
    (test_case_ptr)->dest_h = (dest_h);                         \
    (test_case_ptr)->src_stride = (src_stride);                 \
    (test_case_ptr)->dest_stride = (dest_stride);               \
    (test_case_ptr)->src_unalign_byte = (src_unalign_byte);     \
    (test_case_ptr)->dest_unalign_byte = (dest_unalign_byte);   \
})

// ------------------------------------------------ Static variables ---------------------------------------------------

static const char *TAG_LV_IMAGE_FUNC = "LV Image Functionality";
static char test_msg_buf[200];

static const test_matrix_lv_image_params_t default_test_matrix_image_rgb565_blend_rgb565 = {
#if CONFIG_IDF_TARGET_ESP32S3
    .min_w = 8,                   // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
    .min_h = 1,
    .max_w = 24,
    .max_h = 2,
    .src_max_unalign_byte = 16,   // Use 16-byte boundary check for Xtensa PIE
    .dest_max_unalign_byte = 16,
    .dest_unalign_step = 1,       // Step 1 as the destination array is being aligned in the assembly code all the time
    .src_unalign_step = 3,        // Step 3 (more relaxed) as source array is used unaligned in the assembly code
    .src_stride_step = 3,
    .dest_stride_step = 3,
#else
    .min_w = 1,
    .min_h = 1,
    .max_w = 16,
    .max_h = 2,
    .src_max_unalign_byte = 4,    // Use 4-byte boundary  check for Xtensa base
    .dest_max_unalign_byte = 4,
    .dest_unalign_step = 1,
    .src_unalign_step = 1,
    .src_stride_step = 1,
    .dest_stride_step = 1,
#endif
    .src_min_unalign_byte = 0,
    .dest_min_unalign_byte = 0,
    .test_combinations_count = 0,
};

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Generate all the functionality test combinations
 *
 * - generate functionality test combinations, based on the provided test_matrix struct
 *
 * @param[in] test_matrix Pointer to structure defining test matrix - all the test combinations
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void functionality_test_matrix(test_matrix_lv_image_params_t *test_matrix, func_test_case_lv_image_params_t *test_case);

/**
 * @brief Fill test buffers for image functionality test
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void fill_test_bufs(func_test_case_lv_image_params_t *test_case);

/**
 * @brief The actual functionality test
 *
 * - function prepares structures for functionality testing and runs the LVGL API
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void lv_image_functionality(func_test_case_lv_image_params_t *test_case);

/**
 * @brief Evaluate results of LV Image functionality for 16bit data length
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void test_eval_image_16bit_data(func_test_case_lv_image_params_t *test_case);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Functionality tests

Purpose:
    - Test that an assembly version of LVGL blending API achieves the same results as the ANSI version

Procedure:
    - Prepare testing matrix, to cover all the possible combinations of destination and source arrays widths,
      lengths, strides and memory alignments
    - Run assembly version of the LVGL blending API
    - Run ANSI C version of the LVGL blending API
    - Compare the results
    - Repeat above 3 steps for each test matrix setup
*/

// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("LV Image functionality RGB565 blend to RGB565", "[image][functionality][RGB565]")
{
    test_matrix_lv_image_params_t test_matrix = default_test_matrix_image_rgb565_blend_rgb565;

    func_test_case_lv_image_params_t test_case = {
        .blend_api_func = &lv_draw_sw_blend_image_to_rgb565,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .canary_pixels = CANARY_PIXELS_RGB565,
        .src_data_type_size = sizeof(uint16_t),
        .dest_data_type_size = sizeof(uint16_t),
        .operation_type = OPERATION_FILL,
    };

    ESP_LOGI(TAG_LV_IMAGE_FUNC, "running test for RGB565 color format");
    functionality_test_matrix(&test_matrix, &test_case);
}

// ------------------------------------------------ Static test functions ----------------------------------------------

static void functionality_test_matrix(test_matrix_lv_image_params_t *test_matrix, func_test_case_lv_image_params_t *test_case)
{
    // Step destination array width
    for (int dest_w = test_matrix->min_w; dest_w <= test_matrix->max_w; dest_w++) {

        // Step destination array height
        for (int dest_h = test_matrix->min_h; dest_h <= test_matrix->max_h; dest_h++) {

            // Step source array stride
            for (int src_stride = dest_w; src_stride <= dest_w * 2; src_stride += test_matrix->src_stride_step) {

                // Step destination array stride
                for (int dest_stride = dest_w; dest_stride <= dest_w * 2; dest_stride += test_matrix->dest_stride_step) {

                    // Step source array unalignment
                    for (int src_unalign_byte = test_matrix->src_min_unalign_byte; src_unalign_byte <= test_matrix->src_max_unalign_byte; src_unalign_byte += test_matrix->src_unalign_step) {

                        // Step destination array unalignment
                        for (int dest_unalign_byte = test_matrix->dest_min_unalign_byte; dest_unalign_byte <= test_matrix->dest_max_unalign_byte; dest_unalign_byte += test_matrix->dest_unalign_step) {

                            // Call functionality test
                            UPDATE_TEST_CASE(test_case, dest_w, dest_h, src_stride, dest_stride, src_unalign_byte, dest_unalign_byte);
                            lv_image_functionality(test_case);
                            test_matrix->test_combinations_count++;
                        }
                    }
                }
            }
        }
    }
    ESP_LOGI(TAG_LV_IMAGE_FUNC, "test combinations: %d\n", test_matrix->test_combinations_count);
}

static void lv_image_functionality(func_test_case_lv_image_params_t *test_case)
{
    fill_test_bufs(test_case);

    _lv_draw_sw_blend_image_dsc_t dsc_asm = {
        .dest_buf = test_case->buf.p_dest_asm,
        .dest_w = test_case->dest_w,
        .dest_h = test_case->dest_h,
        .dest_stride = test_case->dest_stride * test_case->dest_data_type_size,  // dest_stride * sizeof(data_type)
        .mask_buf = NULL,
        .mask_stride = 0,
        .src_buf = test_case->buf.p_src,
        .src_stride = test_case->src_stride * test_case->src_data_type_size,     // src_stride * sizeof(data_type)
        .src_color_format = test_case->color_format,
        .opa = LV_OPA_MAX,
        .blend_mode = LV_BLEND_MODE_NORMAL,
        .use_asm = true,
    };

    // Init structure for LVGL blend API, to call the ANSI API
    _lv_draw_sw_blend_image_dsc_t dsc_ansi = dsc_asm;
    dsc_ansi.dest_buf = test_case->buf.p_dest_ansi;
    dsc_ansi.use_asm = false;

    test_case->blend_api_func(&dsc_asm);    // Call the LVGL API with Assembly code
    test_case->blend_api_func(&dsc_ansi);   // Call the LVGL API with ANSI code

    // Shift array pointers by (Canary pixels amount * data type length) back
    test_case->buf.p_dest_asm -= test_case->canary_pixels * test_case->dest_data_type_size;
    test_case->buf.p_dest_ansi -= test_case->canary_pixels * test_case->dest_data_type_size;

    // Evaluate the results
    sprintf(test_msg_buf, "Test case: dest_w = %d, dest_h = %d, dest_stride = %d, src_stride = %d, dest_unalign_byte = %d, src_unalign_byte = %d\n",
            test_case->dest_w, test_case->dest_h, test_case->dest_stride, test_case->src_stride, test_case->dest_unalign_byte, test_case->src_unalign_byte);
#if DBG_PRINT_OUTPUT
    printf("%s\n", test_msg_buf);
#endif
    switch (test_case->color_format) {
    case LV_COLOR_FORMAT_RGB565:
        test_eval_image_16bit_data(test_case);
        break;
    default:
        TEST_ASSERT_MESSAGE(false, "LV Color format not found");
        break;
    }

    // Free memory allocated for test buffers
    free(test_case->buf.p_dest_asm_alloc);
    free(test_case->buf.p_dest_ansi_alloc);
    free(test_case->buf.p_src_alloc);
}

static void fill_test_bufs(func_test_case_lv_image_params_t *test_case)
{
    const size_t src_data_type_size = test_case->src_data_type_size;        // sizeof() of used data type in the source buffer
    const size_t dest_data_type_size = test_case->dest_data_type_size;      // sizeof() of used data type in the destination buffer
    const size_t src_buf_len = test_case->src_buf_len;                      // Total source buffer length, data part of the source buffer including matrix padding (no Canary pixels are used for source buffer)
    const size_t total_dest_buf_len = test_case->total_dest_buf_len;        // Total destination buffer length, data part of the destination buffer including the Canary pixels and matrix padding
    const size_t active_dest_buf_len = test_case->active_dest_buf_len;      // Length of the data part of the destination buffer including matrix padding
    const size_t canary_pixels = test_case->canary_pixels;                    // Canary pixels, according to the data type
    const unsigned int src_unalign_byte = test_case->src_unalign_byte;      // Unalignment bytes for source buffer
    const unsigned int dest_unalign_byte = test_case->dest_unalign_byte;    // Unalignment bytes for destination buffer

    // Allocate destination arrays and source array for Assembly and ANSI LVGL Blend API
    void *src_mem_common = memalign(16, (src_buf_len * src_data_type_size) + src_unalign_byte);
    void *dest_mem_asm   = memalign(16, (total_dest_buf_len * dest_data_type_size) + dest_unalign_byte);
    void *dest_mem_ansi  = memalign(16, (total_dest_buf_len * dest_data_type_size) + dest_unalign_byte);
    TEST_ASSERT_NOT_NULL_MESSAGE(src_mem_common, "Lack of memory");
    TEST_ASSERT_NOT_NULL_MESSAGE(dest_mem_asm, "Lack of memory");
    TEST_ASSERT_NOT_NULL_MESSAGE(dest_mem_ansi, "Lack of memory");

    // Save a pointer to the beginning of the allocated memory which will be used to free()
    test_case->buf.p_src_alloc = src_mem_common;
    test_case->buf.p_dest_asm_alloc = dest_mem_asm;
    test_case->buf.p_dest_ansi_alloc = dest_mem_ansi;

    // Apply destination and source array unalignment
    uint8_t *src_buf_common = (uint8_t *)src_mem_common + src_unalign_byte;
    uint8_t *dest_buf_asm = (uint8_t *)dest_mem_asm + dest_unalign_byte;
    uint8_t *dest_buf_ansi = (uint8_t *)dest_mem_ansi + dest_unalign_byte;

    // Set the whole buffer to 0, including the Canary pixels part
    memset(src_buf_common, 0, src_buf_len * src_data_type_size);
    memset(dest_buf_asm, 0, total_dest_buf_len * src_data_type_size);
    memset(dest_buf_ansi, 0, total_dest_buf_len * src_data_type_size);

    switch (test_case->operation_type) {
    case OPERATION_FILL:
        // Fill the actual part of the destination buffers with known values,
        // Values must be same, because of the stride

        if (test_case->color_format == LV_COLOR_FORMAT_RGB565) {
            uint16_t *dest_buf_asm_uint16 = (uint16_t *)dest_buf_asm;
            uint16_t *dest_buf_ansi_uint16 = (uint16_t *)dest_buf_ansi;
            uint16_t *src_buf_uint16 = (uint16_t *)src_buf_common;

            // Fill destination buffers
            for (int i = 0; i < active_dest_buf_len; i++) {
                dest_buf_asm_uint16[canary_pixels + i] = i + ((i & 1) ? 0x6699 : 0x9966);
                dest_buf_ansi_uint16[canary_pixels + i] = dest_buf_asm_uint16[canary_pixels + i];
            }

            // Fill source buffer
            for (int i = 0; i < src_buf_len; i++) {
                src_buf_uint16[i] = i + ((i & 1) ? 0x55AA : 0xAA55);
            }
        }

        break;
    default:
        TEST_ASSERT_MESSAGE(false, "LV Operation not found");
        break;
    }

    // Shift array pointers by (Canary pixels amount * data type length) forward
    dest_buf_asm += canary_pixels * dest_data_type_size;
    dest_buf_ansi += canary_pixels * dest_data_type_size;

    // Save a pointer to the working part of the memory, where the test data are stored
    test_case->buf.p_src = (void *)src_buf_common;
    test_case->buf.p_dest_asm = (void *)dest_buf_asm;
    test_case->buf.p_dest_ansi = (void *)dest_buf_ansi;

#if DBG_PRINT_OUTPUT
    printf("Destination buffers fill:\n");
    for (uint32_t i = 0; i < test_case->active_dest_buf_len; i++) {
        printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx16" \t asm = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_dest_ansi)[i], ((uint16_t *)test_case->buf.p_dest_asm)[i]);
    }
    printf("\n");

    printf("Source buffer fill:\n");
    for (uint32_t i = 0; i < test_case->src_buf_len; i++) {
        printf("src_buf[%"PRIi32"] %s = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_src)[i]);
    }
    printf("\n");
#endif

}

static void test_eval_image_16bit_data(func_test_case_lv_image_params_t *test_case)
{
    // Print results, 16bit data
#if DBG_PRINT_OUTPUT
    printf("\nEval\nDestination buffers fill:\n");
    for (uint32_t i = 0; i < test_case->total_dest_buf_len; i++) {
        printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx16" \t asm = %8"PRIx16"   %s \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_dest_ansi)[i], ((uint16_t *)test_case->buf.p_dest_asm)[i], (((uint16_t *)test_case->buf.p_dest_ansi)[i] == ((uint16_t *)test_case->buf.p_dest_asm)[i]) ? ("OK") : ("FAIL"));
    }
    printf("\n");

    printf("Source buffer fill:\n");
    for (uint32_t i = 0; i < test_case->src_buf_len; i++) {
        printf("src_buf[%"PRIi32"] %s = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_src)[i]);
    }
    printf("\n");
#endif

    // Canary pixels area must stay 0
    const size_t canary_pixels = test_case->canary_pixels;
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_ansi, canary_pixels, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_asm, canary_pixels, test_msg_buf);

    // dest_buf_asm and dest_buf_ansi must be equal
    TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE((uint16_t *)test_case->buf.p_dest_ansi + canary_pixels, (uint16_t *)test_case->buf.p_dest_asm + canary_pixels, test_case->active_dest_buf_len, test_msg_buf);

    // Data part of the destination buffer and source buffer (not considering matrix padding) must be equal
    uint16_t *dest_row_begin = (uint16_t *)test_case->buf.p_dest_asm + canary_pixels;
    uint16_t *src_row_begin = (uint16_t *)test_case->buf.p_src;
    for (int row = 0; row < test_case->dest_h; row++) {
        TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE(dest_row_begin, src_row_begin, test_case->dest_w, test_msg_buf);
        dest_row_begin += test_case->dest_stride;   // Move pointer of the destination buffer to the next row
        src_row_begin += test_case->src_stride;     // Move pointer of the source buffer to the next row
    }

    // Canary pixels area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_ansi + (test_case->total_dest_buf_len - canary_pixels), canary_pixels, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_asm + (test_case->total_dest_buf_len - canary_pixels), canary_pixels, test_msg_buf);
}
