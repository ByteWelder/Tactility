/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL RGB888 simple fill for ESP32 processor

    .section .text
    .align  4
    .global lv_color_blend_to_rgb888_esp
    .type   lv_color_blend_to_rgb888_esp,@function
// The function implements the following C code:
// void lv_color_blend_to_rgb888(_lv_draw_sw_blend_fill_dsc_t * dsc);

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

lv_color_blend_to_rgb888_esp:

    entry    a1,    32

    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint24_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint16_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff (color)
    l32i.n   a8,    a7,    0                    // a8 - color as value

    // a11 - dest_w_bytes = sizeof(uint24_t) * dest_w = 3 * a4
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = sizeof(uint16_t) * dest_w
    add      a11,   a11,   a4                   // a11 - dest_w_bytes = a11 + a4

    // Prepare register combinations
    // a13 - 0xBBRRGGBB a14 - 0xGGBBRRGG a15 - 0xRRGGBBRR
    l8ui     a13,   a7,    0                     // blue     000B
    slli     a13,   a13,   24                    // shift to B000
    or       a13,   a13,   a8                    // a13      BRGB

    srli     a14,   a8,    8                     // a14      00RG
    slli     a10,   a8,    16                    // a10      GB00
    or       a14,   a14,   a10                   // a14      GBRG

    slli     a15,   a8,    8                     // a15      RGB0
    l8ui     a10,   a7,    2                     // a7       000R
    or       a15,   a15,   a10                   // a15      RGBR

    sub      a6,     a6,    a11                  // dest_stride = dest_stride - dest_w_bytes

    // Prepare main loop length and dest_w_bytes
    srli     a9,     a4,    2                    // a9 = loop_len = dest_w / 4, calculate main loop_len for original dest_w
    movi.n   a8,     0x3                         // a8 = 0x3, remainder mask
    and      a10,    a4,    a8                   // a10 - remainder after division by 4 = a4 and 0x3

    .outer_loop:

        // Run main loop which sets 12 bytes (4 rgb888) in one loop run
        loopnez a9, ._main_loop
            s32i.n      a13,  a3,  0                    // save 32 bits from 32-bit color a13 to dest_buff a3, offset 0
            s32i.n      a14,  a3,  4                    // save 32 bits from 32-bit color a14 to dest_buff a3, offset 4
            s32i.n      a15,  a3,  8                    // save 32 bits from 32-bit color a15 to dest_buff a3, offset 8
            addi.n      a3,   a3,  12                   // increment dest_buff pointer by 12
        ._main_loop:

        bnei   a10,  0x3,  _less_than_3                 // branch if less than 3 values left
            s32i.n      a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
            s32i.n      a14,  a3,  4                    // save 32 bits from a14 to dest_buff a3, offset 4 bytes
            s8i         a15,  a3,  8                    // save  8 bits from a15 to dest_buff a3, offset 8 bytes
            addi.n      a3,   a3,  9                    // increment dest_buff pointer by 9 bytes
            j           _less_than_1
        _less_than_3:

        bnei  a10,  0x2,  _less_than_2                  // branch if less than 2 values left
            s32i.n      a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
            s16i        a14,  a3,  4                    // save 16 bits from a14 to dest_buff a3, offset 4 bytes
            addi.n      a3,   a3,  6                    // increment dest_buff pointer by 6 bytes
            j           _less_than_1
        _less_than_2:

        bnei  a10,  0x1,  _less_than_1                  // branch if less than 1 value left
            s16i        a13,  a3,  0                    // save 16 bits from a13 to dest_buff a3, offset 0 bytes
            s8i         a15,  a3,  2                    // save  8 bits from a15 to dest_buff a3, offset 2 bytes
            addi.n      a3,   a3,  3                    // increment dest_buff pointer by 3 bytes
        _less_than_1:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
        and     a7,  a8,  a3                            // a7 = dest_buff AND 0x3 (check if the address is 4-byte aligned)
    bnez a5, .outer_loop

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return
