/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL ARGB8888 simple fill for ESP32 processor

    .section .text
    .align  4
    .global lv_color_blend_to_argb8888_esp
    .type   lv_color_blend_to_argb8888_esp,@function

// The function implements the following C code:
// void lv_color_blend_to_argb8888(_lv_draw_sw_blend_fill_dsc_t * dsc);

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

lv_color_blend_to_argb8888_esp:

    entry   a1,    32

    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint32_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint32_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff (color)
    l32i.n   a8,    a7,    0                    // a8 - color as value
    slli     a11,   a4,    2                    // a11 - dest_w_bytes = sizeof(uint32_t) * dest_w

    movi     a7,    0xff000000                  // oppactiy mask
    or       a10,    a7,    a8                  // apply oppacity

    srli    a9,    a4,   2                      // a9 - loop_len = dest_w / 4
    sub     a6,    a6,   a11                    // dest_stride = dest_stride - dest_w_bytes

    .outer_loop:

        // Run main loop which sets 16 bytes in one loop run
        loopnez a9, ._main_loop
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3
            s32i.n      a10,  a3,  4                    // save 32 bits from a10 to dest_buff a3
            s32i.n      a10,  a3,  8                    // save 32 bits from a10 to dest_buff a3
            s32i.n      a10,  a3,  12                   // save 32 bits from a10 to dest_buff a3
            addi.n      a3,   a3,  16                   // increment dest_buff pointer by 16 bytes
        ._main_loop:

        // Finish the remaining bytes out of the loop
        // Check modulo 8 of the dest_w_bytes, if - then set 8 bytes
        bbci a11, 3, _mod_8_check                       // branch if 2-nd bit of dest_w_bytes is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            s32i.n      a10,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  8                    // increment dest_buff pointer by 8 bytes
        _mod_8_check:

        // Check modulo 4 of the dest_w_bytes, if - then set 4 bytes
        bbci a11, 2, _mod_4_check                       // branch if 2-nd bit of dest_w_bytes is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
        _mod_4_check:

        add     a3,  a3,  a6                             // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                             // decrease the outer loop
    bnez a5, .outer_loop

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return
