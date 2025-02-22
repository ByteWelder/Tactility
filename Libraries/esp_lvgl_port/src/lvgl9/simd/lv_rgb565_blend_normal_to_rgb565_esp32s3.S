/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lv_macro_memcpy.S"        // Memcpy macros

// This is LVGL RGB565 image blend to RGB565 for ESP32S3 processor

    .section .text
    .align  4
    .global lv_rgb565_blend_normal_to_rgb565_esp
    .type   lv_rgb565_blend_normal_to_rgb565_esp,@function
// The function implements the following C code:
// void lv_color_blend_to_rgb565(_lv_draw_sw_blend_fill_dsc_t * dsc);

// Input params
//
// dsc - a2

// typedef struct {
//     uint32_t opa;                l32i    0
//     void * dst_buf;              l32i    4
//     uint32_t dst_w;              l32i    8
//     uint32_t dst_h;              l32i    12
//     uint32_t dst_stride;         l32i    16
//     const void * src_buf;        l32i    20
//     uint32_t src_stride;         l32i    24
//     const lv_opa_t * mask_buf;   l32i    28
//     uint32_t mask_stride;        l32i    32
// } asm_dsc_t;

lv_rgb565_blend_normal_to_rgb565_esp:

    entry    a1,    32
    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint16_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint16_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff
    l32i.n   a8,    a2,    24                   // a8 - src_stride            in bytes
    movi.n   a10,   0xf                         // 0xf alignment mask (16-byte alignment)
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = sizeof(uint16_t) * dest_w

    // No need to convert any colors here, we are copying from rgb565 to rgb565

    // Check dest_w length
    bltui   a4,  8,  _matrix_width_check                    // Branch if dest_w (a4) is lower than 8

    // Check dest_buff alignment fist
    and     a15,   a10,  a3                                 // 16-byte alignment mask AND dest_buff pointer a3
    bnez    a15,   _src_unalign_dest_unalign                // Branch if a15 not equals to zero
                                                            // Jump straight to the last implementation, since this is the only one which deals with unaligned destination arrays

    // Check src_buff alignment
    and     a15,   a10,  a7                                 // 16-byte alignment mask AND src_buff pointer a7
    bnez    a15,   _src_align_dest_unalign                  // Branch if a15 not equals to zero
                                                            // Jump to check, if the second or third implementation can be used (depends on both strides and dest_w)

    // Check dest_stride alignment
    and     a15,   a10,  a6                                 // 16-byte alignment mask AND dest_stride a6
    bnez    a15,   _src_unalign_dest_unalign                // Branch if a15 not equals to zero
                                                            // Jump straight to the last implementation, since this is the only one which deals with destination stride not aligned

    // Check src_stride alignment
    and     a15,   a10,  a8                                 // 16-byte alignment mask AND src_stride a8
    bnez    a15,   _src_align_dest_unalign                  // Branch if a15 not equals to zero
                                                            // Jump to check, if the second or third implementation can be used (depends on dest_w_bytes)

    // Check dest_w_bytes alignment
    and     a15,   a10,  a11                                // 16-byte alignment mask AND dest_w_bytes
    bnez    a15,   _src_unalign_dest_unalign                // Branch if a15 not equals to zero
                                                            // Jump straight to the last implementation, since this is the only one which deals with dest_w_bytes not aligned

//**********************************************************************************************************************

    // The most ideal case - both arrays aligned, both strides and dest_w are multiples of 16

    // dest_buff   (a3) - 16-byte aligned
    // src_buff    (a7) - 16-byte aligned
    // dest_stride (a6) - 16-byte multiple
    // src_stride  (a8) - 16-byte multiple
    // dest_w      (a4) - 16-byte multiple

    srli    a9,    a4,   4                                  // a9 - loop_len = dest_w / 16
    // Convert strides to matrix paddings
    sub     a6,  a6,  a11                                   // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,  a8,  a11                                   // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_align:

        // Run main loop which copies 32 bytes (16 RGB565 pixels) in one loop run
        loopnez  a9, ._main_loop_align                      // 32 bytes (16 RGB565 pixels) in one loop run
            ee.vld.128.ip q0, a7, 16                        // Load 16 bytes from src_buff a7 to q0, increase src_buf pointer a7 by 16
            ee.vld.128.ip q1, a7, 16                        // Load 16 bytes from src_buff a7 to q1, increase src_buf pointer a7 by 16
            ee.vst.128.ip q0, a3, 16                        // Store 16 bytes from q0 to dest_buff a3, increase dest_buff pointer a3 by 16
            ee.vst.128.ip q1, a3, 16                        // Store 16 bytes from q1 to dest_buff a3, increase dest_buff pointer a3 by 16
        ._main_loop_align:

        // Finish remaining bytes out of the main loop

        // Check modulo 16 of the dest_w, if - then copy 16 bytes (8 RGB565 pixels)
        bbci a11, 4, _align_mod_16_check                    // Branch if 4-th bit of dest_w_bytes a11 is clear
            ee.vld.128.ip q0, a7, 16                        // Load 16 bytes from src_buff a7 to q0, increase src_buf pointer a7 by 16
            ee.vst.128.ip q0, a3, 16                        // Store 16 bytes from q0 to dest_buff a3, increase dest_buff pointer a3 by 16
        _align_mod_16_check:

        add     a3,  a3,  a6                                // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                                // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                                // Decrease the outer loop
    bnez a5, .outer_loop_align

    movi.n   a2, 1                                          // Return LV_RESULT_OK = 1
    retw.n                                                  // Return


    _src_align_dest_unalign:

    // Check dest_stride alignment
    and     a15,   a10,  a6                                 // 16-byte alignment mask AND dest_stride a6
    bnez    a15,   _src_unalign_dest_unalign                // Branch if a15 not equals to zero

    // Check dest_w_bytes alignment
    and     a15,   a10,  a11                                // 16-byte alignment mask AND dest_w_bytes a11
    bnez    a15,   _src_unalign_dest_unalign                // Branch if a15 not equals to zero

    // We don't check src_stride alignment for this implementation, as it can be either align, or unalign

//**********************************************************************************************************************

    // Less ideal case - Only destination array is aligned, src array is unaligned
    //                   Source stride is either aligned or unaligned, destination stride must be aligned, dest_w_bytes must be aligned

    // dest_buff   (a3) - 16-byte aligned
    // src_buff    (a7) - unaligned
    // dest_stride (a6) - 16-byte multiple
    // src_stride  (a8) - does not matter if 16-byte multiple
    // dest_w      (a4) - 16-byte multiple

    // Convert strides to matrix paddings
    sub     a6,  a6,  a11                                   // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,  a8,  a11                                   // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    // Calculate modulo for non-aligned data
    movi    a15,  48                                        // a15 = 48 (main loop copies 48 bytes)
    quou    a9,   a11,  a15                                 // a9 = dest_w_bytes (a11) DIV 48 (15)
    remu    a12,  a11,  a15                                 // a12 = dest_w_bytes (a11) remainder after DIV 48 (15)

    .outer_loop_src_unalign_dest_align:

        ee.ld.128.usar.ip   q2,  a7,  16                    // Preload 16 bytes from src_buff a7 to q2, get value of the SAR_BYTE, increase src_buf pointer a7 by 16
        ee.ld.128.usar.ip   q3,  a7,  16                    // Preload 16 bytes from src_buff a7 to q3, get value of the SAR_BYTE, increase src_buf pointer a7 by 16

        // Run main loop which copies 48 bytes (24 RGB565 pixels) in one loop run
        loopnez a9, ._main_loop_src_unalign_dest_align      // 48 bytes (24 RGB565 pixels) in one loop
            ee.src.q.ld.ip    q4,  a7,  16, q2, q3          // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q.ld.ip    q2,  a7,  16, q3, q4          // Load 16 bytes from src_buff a7 to q2, concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q3,  a3,  16                  // Store 16 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q.ld.ip    q3,  a7,  16, q4, q2          // Load 16 bytes from src_buff a7 to q3, concatenate q4 and q2 and shift to q4 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q4,  a3,  16                  // Store 16 bytes from q4 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
        ._main_loop_src_unalign_dest_align:

        // Finish the main loop outside of the loop from Q registers preloads

        // Check modulo 32 of the loop_len_remainder, if - then copy 32 bytes (16 RGB565 pixels)
        bbci   a12,  5,   _unalign_mod_32_check             // Branch if 5-th bit of loop_len_remainder a12 is clear
            ee.src.q.ld.ip    q4,  a7,  0,  q2, q3          // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, don't increase src_buf pointer a7
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q          q3,  q3,  q4                  // Concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount
            ee.vst.128.ip     q3,  a3,  16                  // Store 16 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            j _end_of_row_src_unalign_dest_align
        _unalign_mod_32_check:

        // Check modulo 16 of the loop_len_remainder, if - then copy 16 bytes (8 RGB565 pixels)
        bbci   a12, 4,   _unalign_mod_16_check              // Branch if 4-th bit of loop_len_remainder a12 is clear
            ee.src.q          q2,  q2,  q3                  // Concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            addi              a7,  a7, -16                  // Correct the src_buff pointer a7, caused by q reg preload
            j _end_of_row_src_unalign_dest_align
        _unalign_mod_16_check:

        // Nothing to copy outside of the main loop
        addi    a7,  a7,  -32                               // Correct the src_buff pointer a7, caused by q reg preload

        _end_of_row_src_unalign_dest_align:

        add     a3,  a3,  a6                                // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                                // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                                // Decrease the outer loop
    bnez a5, .outer_loop_src_unalign_dest_align

    movi.n   a2, 1                                          // Return LV_RESULT_OK = 1
    retw.n                                                  // Return


    _src_unalign_dest_unalign:

//**********************************************************************************************************************

    // The most general case, can handle all the possible combinations

    // dest_buff   (a3) - unaligned
    // src_buff    (a7) - unaligned
    // dest_stride (a6) - not 16-byte multiple
    // src_stride  (a8) - not 16-byte multiple
    // dest_w      (a4) - not 16-byte multiple

    // Convert strides to matrix paddings
    sub     a6,  a6,  a11                                   // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,  a8,  a11                                   // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_all_unalign:

        // dest_buff alignment check
        and     a13,   a10,  a3                             // Alignment mask 0xf (a10) AND dest_buff pointer
        beqz    a13,   _dest_buff_aligned                   // Branch if a13 = 0 (if dest_buff is aligned)

        movi.n  a14,   16                                   // a14 = 16
        sub     a13,   a14,   a13                           // a13 = 16 - unalignment

        // Check modulo 8 of the unalignment a13, if - then copy 8 bytes (4 RGB565 pixels)
        // src_buff a7, dest_buff a3, unalignment a13, copy registers a14, a15
        macro_memcpy_mod_8 a7, a3, a13, a15, a14, __LINE__

        // Check modulo 4 of the unalignment, if - then copy 4 bytes (2 RGB565 pixels)
        // src_buff a7, dest_buff a3, unalignment a13, copy register a15
        macro_memcpy_mod_4 a7, a3, a13, a15, __LINE__

        // Check modulo 2 of the unalignment, if - then copy 2 bytes (1 RGB565 pixel)
        // src_buff a7, dest_buff a3, unalignment a13, copy register a15
        macro_memcpy_mod_2 a7, a3, a13, a15, __LINE__

        // Check modulo 1 of the unalignment, if - then copy 1 byte (1/2 of RGB565 pixel)
        // src_buff a7, dest_buff a3, unalignment a13, copy register a15
        macro_memcpy_mod_1 a7, a3, a13, a15, __LINE__

        _dest_buff_aligned:

        // Calculate modulo for non-aligned data
        sub     a11,   a11,   a13                           // a11 = local_dest_w_bytes (a11) = dest_w_bytes (a11) - (16 - unalignment)
        movi    a15,    48                                  // a15 = 48
        quou    a9,    a11,   a15                           // a9 =  local_dest_w_bytes (a11) DIV 48 (a15)
        remu    a12,   a11,   a15                           // a12 = local_dest_w_bytes (a11) remainder after div 48 (a15)

        ee.ld.128.usar.ip   q2,  a7,  16                    // Preload 16 bytes from src_buff a7 to q2, get value of the SAR_BYTE, increase src_buf pointer a7 by 16
        ee.ld.128.usar.ip   q3,  a7,  16                    // Preload 16 bytes from src_buff a7 to q3, get value of the SAR_BYTE, increase src_buf pointer a7 by 16

        // Run main loop which copies 48 bytes (24 RGB565 pixels) in one loop run
        loopnez a9, ._main_loop_all_unalign                 // 48 bytes (24 RGB565 pixels) in one loop
            ee.src.q.ld.ip    q4,  a7,  16, q2, q3          // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q.ld.ip    q2,  a7,  16, q3, q4          // Load 16 bytes from src_buff a7 to q2, concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q3,  a3,  16                  // Store 16 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q.ld.ip    q3,  a7,  16, q4, q2          // Load 16 bytes from src_buff a7 to q3, concatenate q4 and q2 and shift to q4 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q4,  a3,  16                  // Store 16 bytes from q4 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
        ._main_loop_all_unalign:

        // Finish the main loop outside of the loop from Q registers preloads

        // Check modulo 32 and modulo 8 of the loop_len_remainder a12
        bbci   a12,  5,   _all_unalign_mod_32_check         // Branch if 5-th bit of loop_len_remainder a12 is clear
        bbsi   a12,  3,   _all_unalign_mod_32_mod_8_check   // Branch if 3-rd bif of loop_len_remainder a12 is set

            // Copy 32 bytes (16 RGB565 pixels) (47 - 40)
            ee.src.q.ld.ip    q4,  a7,  0,  q2, q3          // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, don't increase src_buf pointer a7
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q          q3,  q3,  q4                  // Concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount
            ee.vst.128.ip     q3,  a3,  16                  // Store 16 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            j _skip_mod16

            _all_unalign_mod_32_mod_8_check:
            // Copy 40 bytes (20 RGB565 pixels)
            ee.src.q.ld.ip    q4,  a7,  16, q2, q3          // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, increase src_buf pointer a7 by 16
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q.ld.ip    q2,  a7,  0,  q3, q4          // Load 16 bytes from src_buff a7 to q2, concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount, don't increase src_buf pointer a7
            ee.vst.128.ip     q3,  a3,  16                  // Store 16 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q          q4,  q4,  q2                  // Concatenate q4 and q2 and shift to q4 by the SAR_BYTE amount
            ee.vst.l.64.ip    q4,  a3,  8                   // Store lower 8 bytes from q4 to aligned dest_buff a3, increase dest_buff pointer a3 by 8
            addi              a7,  a7,  -8                  // Correct the src_buff pointer a7, caused by q reg preload
            j _skip_mod16

        _all_unalign_mod_32_check:

        // Check modulo 16 and modulo 8 of the loop_len_remainder a12
        bbci   a12,  4,   _all_unalign_mod_16_check         // branch if 4-th bit of loop_len_remainder a12 is clear
        bbsi   a12,  3,   _all_unalign_mod_16_mod_8_check   // branch if 3-rd bit of loop_len_remainder a12 is set

            // Copy 16 bytes (8 RGB565 pixels)
            ee.src.q          q2,  q2,  q3                  // Concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            addi              a7,  a7, -16                  // Correct the src_buff pointer a7, caused by q reg preload
            j _skip_mod16

            _all_unalign_mod_16_mod_8_check:
            // Copy 24 bytes (12 RGB565 pixels)
            ee.src.q.ld.ip    q4,  a7,  0, q2, q3           // Load 16 bytes from src_buff a7 to q4, concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount, don't increase src_buf pointer a7
            ee.vst.128.ip     q2,  a3,  16                  // Store 16 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 16
            ee.src.q          q3,  q3,  q4                  // Concatenate q3 and q4 and shift to q3 by the SAR_BYTE amount
            ee.vst.l.64.ip    q3,  a3,  8                   // Store lower 8 bytes from q3 to aligned dest_buff a3, increase dest_buff pointer a3 by 8
            addi              a7,  a7, -8                   // Correct the src_buff pointer a7, caused by q reg preload
            j _skip_mod16
        _all_unalign_mod_16_check:

        bbci   a12, 3,  _all_unalign_mod_8_check            // Branch if 3-rd bit of loop_len_remainder a12 is clear
            // Copy 8 bytes (4 RGB565 pixels)
            ee.src.q          q2,  q2,  q3                  // Concatenate q2 and q3 and shift to q2 by the SAR_BYTE amount
            ee.vst.l.64.ip    q2,  a3,  8                   // Store lower 8 bytes from q2 to aligned dest_buff a3, increase dest_buff pointer a3 by 8
            addi              a7,  a7,  -24                 // Correct the src_buff pointer a7, caused by q reg preload
            j _skip_mod16
        _all_unalign_mod_8_check:

        addi    a7, a7, -32                                 // Correct the src_buff pointer a7, caused by q reg preload

        _skip_mod16:

        // Check modulo 4 of the loop_len_remainder, if - then copy 4 bytes (2 RGB565 pixels)
        // src_buff a7, dest_buff a3, loop_len_remainder a12, copy register a15
        macro_memcpy_mod_4 a7, a3, a12, a15, __LINE__

        // Check modulo 2 of the loop_len_remainder, if - then copy 2 bytes (1 RGB565 pixel)
        // src_buff a7, dest_buff a3, loop_len_remainder a12, copy register a15
        macro_memcpy_mod_2 a7, a3, a12, a15, __LINE__

        // Check modulo 1 of the loop_len_remainder, if - then copy 1 byte (1/2 RGB565 pixel)
        // src_buff a7, dest_buff a3, loop_len_remainder a12, copy register a15
        macro_memcpy_mod_1 a7, a3, a12, a15, __LINE_

        slli    a11, a4,   1                                // Refresh dest_w_bytes
        add     a3,  a3,  a6                                // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                                // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                                // Decrease the outer loop
    bnez a5, .outer_loop_all_unalign

    movi.n   a2, 1                                          // Return LV_RESULT_OK = 1
    retw.n                                                  // Return

//**********************************************************************************************************************

    // Small matrix width, keep it simple for lengths less than 8 pixels
    _matrix_width_check:                                    // Matrix width is greater or equal 8 pixels

    // Convert strides to matrix paddings
    sub     a6,  a6,  a11                                   // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,  a8,  a11                                   // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_short_matrix_length:

        // Run main loop which copies 2 bytes (one RGB565 pixel) in one loop run
        loopnez a4, ._main_loop_short_matrix_length
            l8ui        a15,  a7,  0                        // Load 8 bits from src_buff a7 to a15, offset 0
            l8ui        a14,  a7,  1                        // Load 8 bits from src_buff a7 to a14, offset 1
            s8i         a15,  a3,  0                        // Save 8 bits from a15 to dest_buff a3, offset 0
            s8i         a14,  a3,  1                        // Save 8 bits from a14 to dest_buff a3, offset 1
            addi.n      a7,   a7,  2                        // Increment src_buff pointer a7 by 1
            addi.n      a3,   a3,  2                        // Increment dest_buff pointer a3 by 2
        ._main_loop_short_matrix_length:

        // Finish remaining byte out of the main loop

        // Check modulo 1 of the dest_w_bytes (a11), if - then copy 1 byte (1/2 RGB565 pixel)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy register a15
        macro_memcpy_mod_1 a7, a3, a11, a15, __LINE__

        add     a3,  a3,  a6                                // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                                // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                                // Decrease the outer loop
    bnez a5, .outer_loop_short_matrix_length

    movi.n   a2, 1                                          // Return LV_RESULT_OK = 1
    retw.n                                                  // Return
