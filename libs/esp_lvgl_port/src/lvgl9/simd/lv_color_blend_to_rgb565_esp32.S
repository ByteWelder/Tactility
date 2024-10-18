/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL RGB565 simple fill for ESP32 processor

    .section .text
    .align  4
    .global lv_color_blend_to_rgb565_esp
    .type   lv_color_blend_to_rgb565_esp,@function
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

lv_color_blend_to_rgb565_esp:

    entry   a1,    32

    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint16_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint16_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff (color)
    l32i.n   a8,    a7,    0                    // a8 - color as value
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = sizeof(uint16_t) * dest_w

    // Convert color to rgb656
    l8ui    a15,    a7,    2                    // red
    movi.n  a14,    0xf8
    and     a13,    a15,   a14
    slli    a10,    a13,   8

    l8ui    a15,    a7,    0                    // blue
    and     a13,    a15,   a14
    srli    a12,    a13,   3
    add     a10,    a10,   a12

    l8ui    a15,    a7,    1                    // green
    movi.n  a14,    0xfc
    and     a13,    a15,   a14
    slli    a12,    a13,   3
    add     a12,    a10,   a12                  // a12 = 16-bit color

    slli    a10,    a12,   16
    movi.n  a13,    0xFFFF0000
    and     a10,    a10,    a13
    or      a10,    a10,    a12                 // a10 = 32-bit color (16bit + (16bit << 16))

    movi.n  a8,    0x3                          // a8 = 0x3, dest_buff align mask
    sub     a6,    a6,   a11                    // dest_stride = dest_stride - dest_w_bytes

    // cache init
    // Prepare main loop length and dest_w_bytes
    srli     a9,     a4,    4                    // a9 = loop_len = dest_w / 8, calculate main loop_len for original dest_w
    slli     a11,    a4,    1                    // a11 = dest_w_bytes = sizeof(uint16_t) * dest_w
    addi     a4,     a4,   -1                    // a4-- (decrement a4)
    s32i.n   a9,     a1,    0                    // cache.orig.loop_len
    s32i.n   a11,    a1,    4                    // cache.orig.dest_w_bytes

    // Prepare decreased main loop length and dest_w_bytes
    srli     a9,     a4,    4                    // a9 = loop_len = dest_w / 8, calculate main loop_len for dest_w - 1
    slli     a11,    a4,    1                    // a11 = dest_w_bytes = sizeof(uint16_t) * (dest_w - 1)
    s32i.n   a9,     a1,    8                    // cache.decr.loop_len
    s32i.n   a11,    a1,    12                   // cache.decr.dest_w_bytes
    and      a7,     a8,    a3                   // a7 = dest_buff AND 0x3 (chck if the address is 4-byte aligned)

    .outer_loop:

        // Check if the des_buff is 2-byte aligned
        beqz  a7, _dest_buff_2_byte_aligned      // branch if a7 is equal to zero
            s16i        a12,  a3,   0            // save 16 bits from 16-bit color a12 to dest_buff a3, offset 0
            l32i.n      a9,   a1,   8            // a9  = load cache.decr.loop_len
            l32i.n      a11,  a1,   12           // a11 = load cache.decr.dest_w_bytes
            addi.n      a3,   a3,   2            // increment dest_buff pointer by 2
            j           _dest_buff_unaligned
        _dest_buff_2_byte_aligned:

        l32i.n      a9,   a1,   0                // a11 = load cache.orig.loop_len
        l32i.n      a11,  a1,   4                // a11 = load cache.orig.dest_w_bytes

        _dest_buff_unaligned:

        // Run main loop which sets 16 bytes in one loop run
        loopnez a9, ._main_loop
            s32i.n      a10,  a3,  0                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 0
            s32i.n      a10,  a3,  4                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 4
            s32i.n      a10,  a3,  8                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 8
            s32i.n      a10,  a3,  12                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 12
            s32i.n      a10,  a3,  16                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 16
            s32i.n      a10,  a3,  20                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 20
            s32i.n      a10,  a3,  24                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 24
            s32i.n      a10,  a3,  28                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 28
            addi.n      a3,   a3,  32                   // increment dest_buff pointer by 32
        ._main_loop:

        // Finish the remaining bytes out of the loop
        // Check modulo 8 of the dest_w_bytes, if - then set 16 bytes
        bbci a11, 4, _mod_16_check                      // branch if 2-nd bit of dest_w_bytes is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 0
            s32i.n      a10,  a3,  4                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 4
            s32i.n      a10,  a3,  8                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 8
            s32i.n      a10,  a3,  12                   // save 32 bits from 32-bit color a10 to dest_buff a3, offset 12
            addi.n      a3,   a3,  16                   // increment dest_buff pointer by 16
        _mod_16_check:

        // Finish the remaining bytes out of the loop
        // Check modulo 8 of the dest_w_bytes, if - then set 8 bytes
        bbci a11, 3, _mod_8_check                       // branch if 2-nd bit of dest_w_bytes is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 0
            s32i.n      a10,  a3,  4                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 4
            addi.n      a3,   a3,  8                    // increment dest_buff pointer by 8 bytes
        _mod_8_check:

        // Check modulo 4 of the dest_w_bytes, if - then set 4 bytes
        bbci a11, 2, _mod_4_check                       // branch if 2-nd bit of dest_w_bytes is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from 32-bit color a10 to dest_buff a3, offset 0
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4
        _mod_4_check:

        // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
        bbci a11, 1, _mod_2_check                       // branch if 1-st bit of dest_w_bytes is clear
            s16i        a12,  a3,  0                    // save 16 bits from 16-bit color a12 to dest_buff a3, offset 0
            addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
        _mod_2_check:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
        and     a7,  a8,  a3                            // a7 = dest_buff AND 0x3 (chck if the address is 4-byte aligned)
    bnez a5, .outer_loop

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return
