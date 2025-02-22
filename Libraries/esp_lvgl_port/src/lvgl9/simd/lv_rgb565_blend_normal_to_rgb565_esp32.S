/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lv_macro_memcpy.S"        // Memcpy macros

// This is LVGL RGB565 image blend to RGB565 for ESP32 processor

    .section .text
    .align  4
    .global lv_rgb565_blend_normal_to_rgb565_esp
    .type   lv_rgb565_blend_normal_to_rgb565_esp,@function
// The function implements the following C code:
// void rgb565_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc);

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
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = sizeof(uint16_t) * dest_w

    // No need to convert any colors here, we are copying from rgb565 to rgb565

    // Check dest_w length
    bltui   a4,  8,  _matrix_width_check                // Branch if dest_w (a4) is lower than 8

    // Check memory alignment and input parameters lengths and decide which implementation to use
    movi.n   a10,   0x3                                 // a10 = 0x3 alignment mask (4-byte alignment)
    or       a15,   a7,    a3                           // a15 = src_buff (a7) OR dest_buff (a3)
    or       a15,   a15,   a6                           // a15 = a15 OR dest_stride (a6)
    or       a15,   a15,   a8                           // a15 = a15 OR src_stride (a8)
    or       a15,   a15,   a11                          // a15 = a15 OR dest_w_bytes (a11)
    and      a15,   a15,   a10                          // a15 = a15 AND alignment mask (a10)
    bnez     a15,   _alignment_check                    // Branch if a15 not equals to zero

//**********************************************************************************************************************

    // The most ideal case - both arrays aligned, both strides and dest_w are multiples of 4

    // dest_buff   (a3) - 4-byte aligned
    // src_buff    (a7) - 4-byte aligned
    // dest_stride (a6) - 4-byte multiple
    // src_stride  (a8) - 4-byte multiple
    // dest_w      (a4) - 4-byte multiple

    srli    a9,    a4,   3                              // a9 - loop_len = dest_w / 8
    // Convert strides to matrix paddings
    sub     a6,    a6,   a11                            // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,    a8,   a11                            // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_align:

        // Run main loop which copies 16 bytes (8 RGB565 pixels) in one loop run
        loopnez a9, ._main_loop_aligned
            l32i.n      a15,  a7,  0                    // Load 32 bits from src_buff a7 to a15, offset 0
            l32i.n      a14,  a7,  4                    // Load 32 bits from src_buff a7 to a14, offset 4
            l32i.n      a13,  a7,  8                    // Load 32 bits from src_buff a7 to a13, offset 8
            l32i.n      a12,  a7,  12                   // Load 32 bits from src_buff a7 to a12, offset 12
            s32i.n      a15,  a3,  0                    // Save 32 bits from a15 to dest_buff a3, offset 0
            s32i.n      a14,  a3,  4                    // Save 32 bits from a15 to dest_buff a3, offset 4
            s32i.n      a13,  a3,  8                    // Save 32 bits from a15 to dest_buff a3, offset 8
            s32i.n      a12,  a3,  12                   // Save 32 bits from a15 to dest_buff a3, offset 12
            addi.n      a7,   a7,  16                   // Increment src_buff pointer a7 by 16
            addi.n      a3,   a3,  16                   // Increment dest_buff pointer a3 by 16
        ._main_loop_aligned:

        // Finish the remaining bytes out of the main loop

        // Check modulo 8 of the dest_w_bytes (a11), if - then copy 8 bytes (4 RGB565 pixels)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy registers a14 a15
        macro_memcpy_mod_8 a7, a3, a11, a14, a15 __LINE__

        // Check modulo 4 of the dest_w_bytes (a11), if - then copy 4 bytes (2 RGB565 pixels)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy register a15
        macro_memcpy_mod_4 a7, a3, a11, a15 __LINE__

        // Check modulo 2 of the dest_w_bytes (a11), if - then copy 2 bytes (1 RGB565 pixel)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy register a15
        macro_memcpy_mod_2 a7, a3, a11, a15 __LINE__

        // Check modulo 1 of the dest_w_bytes (a11), if - then copy 1 byte (1/2 RGB565 pixel)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy register a15
        macro_memcpy_mod_1 a7, a3, a11, a15 __LINE__

        add     a3,  a3,  a6                            // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                            // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                            // Decrease the outer loop
    bnez a5, .outer_loop_align

    movi.n   a2, 1                                      // Return LV_RESULT_OK = 1
    retw.n                                              // Return


//**********************************************************************************************************************

    // The most general case - at leas one array is not aligned, or one parameter is not multiple of 4
    _alignment_check:

    // dest_buff   (a3) - 4-byte aligned, or not
    // src_buff    (a7) - 4-byte aligned, or not
    // dest_stride (a6) - 4-byte multiple, or not
    // src_stride  (a8) - 4-byte multiple, or not
    // dest_w      (a4) - 4-byte multiple, or not

    // Convert strides to matrix paddings
    sub     a6,    a6,   a11                            // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,    a8,   a11                            // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_unalign:

        extui       a13,  a3,   0,   2                  // Get last two bits of the dest_buff address a3, to a13
        movi.n      a15,  4                             // Move 4 to a15, for calculation of the destination alignment loop
        sub         a14,  a15,  a13                     // Calculate destination alignment loop length (a14 = 4 - a13)

        // In case of the dest_buff a3 being already aligned (for example by matrix padding), correct a14 value,
        // to prevent the destination aligning loop to run 4 times (to prevent aligning already aligned memory)
        moveqz      a14,  a13,  a13                     // If a13 is zero, move a13 to a14, move 0 to a14

        sub         a10,  a11,  a14                     // Get the dest_w_bytes after the aligning loop
        srli        a9,   a10,  4                       // Calculate main loop len (a9 = dest_w_bytes_local / 16)

        // Run dest_buff aligning loop byte by byte
        loopnez a14, ._dest_aligning_loop
            l8ui        a15,  a7,  0                    // Load 8 bits from src_buff a7 to a15, offset 0
            addi.n      a7,   a7,  1                    // Increment src_buff pointer a7 by 1
            s8i         a15,  a3,  0                    // Save 8 bits from a15 to dest_buff a3, offset 0
            addi.n      a3,   a3,  1                    // Increment dest_buff pointer a3 by 1
        ._dest_aligning_loop:

        // Destination is aligned, source is unaligned

        // For more information about this implementation, see chapter 3.3.2 Shifts and the Shift Amount Register (SAR)
        // in Xtensa Instruction Set Architecture (ISA) Reference Manual

        ssa8l       a7                                  // Set SAR_BYTE from src_buff a7 unalignment
        extui       a4,  a7,  0,  2                     // Get last 2 bits of the src_buff,  a4 = src_buff_unalignment 
        sub         a7,  a7,  a4                        // "align" the src_buff a7, to 4-byte boundary by decreasing it's pointer to the nearest aligned boundary

        // First preload for the loopnez cycle
        l32i.n      a15,  a7,  0                        // Load 32 bits from 4-byte aligned src_buff a7 to a15, offset 0

        // Run main loop which copies 16 bytes (8 RGB565 pixels) in one loop run
        loopnez a9, ._main_loop_unalign
            l32i.n      a14,  a7,   4                   // Load 32 bits from 4-byte aligned src_buff a7 to a14, offset 4
            l32i.n      a13,  a7,   8                   // Load 32 bits from 4-byte aligned src_buff a7 to a13, offset 8
            src         a15,  a14,  a15                 // Concatenate a14 and a15 and shift by SAR_BYTE amount to a15
            s32i.n      a15,  a3,   0                   // Save 32 bits from shift-corrected a15 to dest_buff a3, offset 0
            l32i.n      a12,  a7,   12                  // Load 32 bits from 4-byte aligned src_buff a7 to a12, offset 12
            src         a14,  a13,  a14                 // Concatenate a13 and a14 and shift by SAR_BYTE amount to a14
            s32i.n      a14,  a3,   4                   // Save 32 bits from shift-corrected a14 to dest_buff a3, offset 4
            l32i.n      a15,  a7,   16                  // Load 32 bits from 4-byte aligned src_buff a7 to a15, offset 16
            src         a13,  a12,  a13                 // Concatenate a12 and a13 and shift by SAR_BYTE amount to a13
            s32i.n      a13,  a3,   8                   // Save 32 bits from shift-corrected a13 to dest_buff a3, offset 8
            addi.n      a7,   a7,   16                  // Increment src_buff pointer a7 by 16
            src         a12,  a15,  a12                 // Concatenate a15 and a12 and shift by SAR_BYTE amount to a12
            s32i.n      a12,  a3,   12                  // Save 32 bits from shift-corrected a12 to dest_buff a3, offset 12
            addi.n      a3,   a3,   16                  // Increment dest_buff pointer a3 by 16
        ._main_loop_unalign:

        // Finish the remaining bytes out of the loop
        // Check modulo 8 of the dest_w_bytes_local (a10), if - then copy 8 bytes
        bbci a10, 3, _mod_8_check                       // Branch if 3-rd bit of dest_w_bytes_local is clear
            l32i.n      a14,  a7,   4                   // Load 32 bits from 4-byte aligned src_buff a7 to a14, offset 4
            l32i.n      a13,  a7,   8                   // Load 32 bits from 4-byte aligned src_buff a7 to a13, offset 8
            src         a15,  a14,  a15                 // Concatenate a14 and a15 and shift by SAR_BYTE amount to a15 (value in a15 is already prepared from previous steps)
            s32i.n      a15,  a3,   0                   // Save 32 bits from shift-corrected a15 to dest_buff a3, offset 0
            addi.n      a7,   a7,   8                   // Increment src_buff pointer a7 by 8
            src         a14,  a13,  a14                 // Concatenate a13 and a14 and shift by SAR_BYTE amount to a14
            s32i.n      a14,  a3,   4                   // Save 32 bits from shift-corrected a14 to dest_buff a3, offset 4
            addi.n      a3,   a3,   8                   // Increment dest_buff pointer a3 by 8
            mov         a15,  a13                       // Prepare a15 for the next steps (copy a13 to a15)
        _mod_8_check:

        // Check modulo 4 of the dest_w_bytes_local (a10), if - then copy 4 bytes
        bbci a10, 2, _mod_4_check                       // Branch if 2-nd bit of dest_w_bytes_local is clear
            l32i.n      a14,  a7,   4                   // Load 32 bits from 4-byte aligned src_buff a7 to a14, offset 4
            addi.n      a7,   a7,   4                   // Increment src_buff pointer a7 by 4
            src         a15,  a14,  a15                 // Concatenate a14 and a15 and shift by SAR_BYTE amount to a15 (value in a15 is already prepared from previous steps)
            s32i.n      a15,  a3,   0                   // Save 32 bits from shift-corrected a15 to dest_buff a3, offset 0
            addi.n      a3,   a3,   4                   // Increment dest_buff pointer a3 by 4
            mov         a15,  a14                       // Prepare a15 for the next steps (copy a14 to a15)
        _mod_4_check:

        extui       a13,  a10,  0,  2                   // Get the last 2 bytes of the dest_w_bytes_local (a10), a13 = a10[1:0], to find out how many bytes are needs copied and to increase src and dest pointer accordingly
        beqz        a13,  _mod_1_2_check                // Branch if a13 equal to zero, E.G. if there are no bytes to be copied
            l32i.n      a14,  a7,   4                   // Load 32 bits from 4-byte aligned src_buff a7 to a14, offset 4
            l32i.n      a12,  a3,   0                   // Get dest_buff value: Load 32 bits from 4-byte aligned dest_buff a3 to a12, offset 0
            src         a15,  a14,  a15                 // Concatenate a14 and a15 and shift by SAR_BYTE amount to a15 (value in a15 is already prepared from previous steps)
            ssa8l       a10                             // Set SAR_BYTE from dest_w_bytes_local a10 length
            sll         a15,  a15                       // Shift the dest word a15 by SAR_BYTE amount
            srl         a12,  a12                       // Shift the src word a12 by SAR_BYTE amount
            ssa8b       a10                             // Set SAR_BYTE from dest_w_bytes_local a10 length
            src         a12,  a12,  a15                 // Concatenate a12 and a15 and shift by SAR_BYTE amount to a12
            s32i.n      a12,  a3,   0                   // Save 32 bits from shift-corrected a12 to dest_buff a3, offset 0
            add         a7,   a7,   a13                 // Increment src_buff pointer a7, by amount of copied bytes (a13)
            add         a3,   a3,   a13                 // Increment dest_buff pointer a3, by amount of copied bytes (a13)
        _mod_1_2_check:

        add     a7,  a7,  a4                            // Correct the src_buff back by src_buff_unalignment (a4), after we have force-aligned it to 4-byte boundary before the main loop
        add     a3,  a3,  a6                            // dest_buff + dest_stride
        add     a7,  a7,  a8                            // src_buff + src_stride
        addi.n  a5,  a5,  -1                            // Decrease the outer loop
    bnez a5, .outer_loop_unalign

    movi.n   a2, 1                                      // Return LV_RESULT_OK = 1
    retw.n                                              // Return

//**********************************************************************************************************************

    // Small matrix width, keep it simple for lengths less than 8 pixels
    _matrix_width_check:                                // Matrix width is greater or equal 8 pixels

    // Convert strides to matrix paddings
    sub     a6,  a6,  a11                               // dest_matrix_padding (a6) = dest_stride (a6) - dest_w_bytes (a11)
    sub     a8,  a8,  a11                               // src_matrix_padding (a8) = src_stride (a8) - dest_w_bytes (a11)

    .outer_loop_short_matrix_length:

        // Run main loop which copies 2 bytes (one RGB565 pixel) in one loop run
        loopnez a4, ._main_loop_short_matrix_length
            l8ui        a15,  a7,  0                    // Load 8 bits from src_buff a7 to a15, offset 0
            l8ui        a14,  a7,  1                    // Load 8 bits from src_buff a7 to a14, offset 1
            s8i         a15,  a3,  0                    // Save 8 bits from a15 to dest_buff a3, offset 0
            s8i         a14,  a3,  1                    // Save 8 bits from a14 to dest_buff a3, offset 1
            addi.n      a7,   a7,  2                    // Increment src_buff pointer a7 by 1
            addi.n      a3,   a3,  2                    // Increment dest_buff pointer a3 by 2
        ._main_loop_short_matrix_length:

        // Finish remaining byte out of the main loop

        // Check modulo 1 of the dest_w_bytes (a11), if - then copy 1 byte (1/2 RGB565 pixel)
        // src_buff a7, dest_buff a3, dest_w_bytes a11, copy register a15
        macro_memcpy_mod_1 a7, a3, a11, a15, __LINE__

        add     a3,  a3,  a6                            // dest_buff (a3) = dest_buff (a3) + dest_matrix_padding (a6)
        add     a7,  a7,  a8                            // src_buff (a7) = src_buff (a7) + src_matrix_padding (a8)
        addi.n  a5,  a5,  -1                            // Decrease the outer loop
    bnez a5, .outer_loop_short_matrix_length

    movi.n   a2, 1                                      // Return LV_RESULT_OK = 1
    retw.n                                              // Return
