/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "lv_color.h"
#include "lv_draw_sw_blend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------- Macros and Types --------------------------------------------------

/**
 * @brief Type of blend DUT function
 */
typedef enum {
    OPERATION_FILL,
    OPERATION_FILL_WITH_OPA,
} blend_operation_t;

/**
 * @brief Canary pixels amount depending on data type
 * @note
 *    - We should use at least 16 bytes of memory for canary pixels because of esp32s3 TIE 16-bytes wide Q registers
 *    - Canary pixels are multiplied by sizeof(used_data_type) to get the memory length occupied by the canary pixels
 *    - The memory occupied by canary pixels should be in 16-byte multiples, to achieve 16-byte memory alignment in functionality test
 *    - For example, ideally, for RGB565 we would need 8 canary pixels -> 8 * sizeof(uint16_t) = 16
 */
typedef enum {
    CANARY_PIXELS_ARGB8888 = 4,                                /*!< Canary pixels: 4 * sizeof(uint32_t) = 16 */
    CANARY_PIXELS_RGB565 = 8,                                  /*!< Canary pixels: 8 * sizeof(uint16_t) = 16 */
} canary_pixels_t;

/**
 * @brief Functionality test combinations for LV Image
 */
typedef struct {
    unsigned int min_w;                                       /*!< Minimum width of the test array */
    unsigned int min_h;                                       /*!< Minimum height of the test array */
    unsigned int max_w;                                       /*!< Maximum width of the test array */
    unsigned int max_h;                                       /*!< Maximum height of the test array */
    unsigned int src_min_unalign_byte;                        /*!< Minimum amount of unaligned bytes of the source test array */
    unsigned int dest_min_unalign_byte;                       /*!< Minimum amount of unaligned bytes of the destination test array */
    unsigned int src_max_unalign_byte;                        /*!< Maximum amount of unaligned bytes of the source test array */
    unsigned int dest_max_unalign_byte;                       /*!< Maximum amount of unaligned bytes of the destination test array */
    unsigned int src_unalign_step;                            /*!< Increment step in bytes unalignment of the source test array */
    unsigned int dest_unalign_step;                           /*!< Increment step in bytes unalignment of the destination test array */
    unsigned int src_stride_step;                             /*!< Increment step in destination stride of the source test array */
    unsigned int dest_stride_step;                            /*!< Increment step in destination stride of the destination test array */
    unsigned int test_combinations_count;                     /*!< Count of fest combinations */
} test_matrix_lv_image_params_t;


/**
 * @brief Functionality test case parameters for LV Image
 */
typedef struct {
    struct {
        void *p_src;                                          /*!< pointer to the source test buff (common src buffer for both the ANSI and ASM)  */
        void *p_src_alloc;                                    /*!< pointer to the beginning of the memory allocated for the source ASM test buf, used in free() */
        void *p_dest_asm;                                     /*!< pointer to the destination ASM test buf */
        void *p_dest_ansi;                                    /*!< pointer to the destination ANSI test buf */
        void *p_dest_asm_alloc;                               /*!< pointer to the beginning of the memory allocated for the destination ASM test buf, used in free() */
        void *p_dest_ansi_alloc;                              /*!< pointer to the beginning of the memory allocated for the destination ANSI test buf, used in free() */
    } buf;
    void (*blend_api_func)(_lv_draw_sw_blend_image_dsc_t *);  /*!< pointer to LVGL API function */
    lv_color_format_t color_format;                           /*!< LV color format */
    size_t src_data_type_size;                                /*!< Used data type size in the source buffer, eg sizeof(src_buff[0]) */
    size_t dest_data_type_size;                               /*!< Used data type size in the destination buffer, eg sizeof(dest_buff[0]) */
    size_t src_buf_len;                                       /*!< Length of the source buffer, including matrix padding (no Canary pixels are used for source buffer) */
    size_t active_dest_buf_len;                               /*!< Length of the destination buffer, where the actual data are stored, including matrix padding, not including Canary pixels */
    size_t total_dest_buf_len;                                /*!< Total length of the destination buffer (including Canary pixels and matrix padding) */
    size_t canary_pixels;                                     /*!< Canary pixels must be adjusted according to the used color type, to achieve aligned memory effect */
    unsigned int dest_w;                                      /*!< Destination buffer width */
    unsigned int dest_h;                                      /*!< Destination buffer height */
    unsigned int src_stride;                                  /*!< Source buffer stride */
    unsigned int dest_stride;                                 /*!< Destination buffer stride */
    unsigned int src_unalign_byte;                            /*!< Source buffer memory unalignment */
    unsigned int dest_unalign_byte;                           /*!< Destination buffer memory unalignment */
    blend_operation_t operation_type;                         /*!< Type of fundamental blend operation */
} func_test_case_lv_image_params_t;


/**
 * @brief Benchmark test case parameters for LV Image
 */
typedef struct {
    unsigned int height;                                      /*!< Test array height */
    unsigned int width;                                       /*!< Test array width */
    unsigned int dest_stride;                                 /*!< Destination test array stride */
    unsigned int src_stride;                                  /*!< Source test array stride */
    unsigned int cc_height;                                   /*!< Corner case test array height */
    unsigned int cc_width;                                    /*!< Corner case test array width */
    unsigned int benchmark_cycles;                            /*!< Count of benchmark cycles */
    void *src_array_align16;                                  /*!< Source test array with 16 byte alignment - testing most ideal case */
    void *src_array_align1;                                   /*!< Source test array with 1 byte alignment - testing worst case */
    void *dest_array_align16;                                 /*!< Destination test array with 16 byte alignment - testing most ideal case */
    void *dest_array_align1;                                  /*!< Destination test array with 1 byte alignment - testing worst case */
    void (*blend_api_func)(_lv_draw_sw_blend_image_dsc_t *);  /*!< pointer to LVGL API function */
} bench_test_case_lv_image_params_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif
