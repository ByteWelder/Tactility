/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL RGB565 simple fill for ESP32S3 processor

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

    entry    a1,    32

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

    // Check for short lengths
    // dest_w should be at least 16, othewise it's not worth using esp32s3 TIE
    bgei     a4,   16,  _esp32s3_implementation         // Branch if dest_w is greater than or equal to 16
    j .lv_color_blend_to_rgb565_esp32_body              // Jump to esp32 implementation

    _esp32s3_implementation:

    ee.movi.32.q   q0,   a10,  0                        // fill q0 register from a10 by 32 bits
    ee.movi.32.q   q0,   a10,  1
    ee.movi.32.q   q0,   a10,  2
    ee.movi.32.q   q0,   a10,  3

    // Check dest_buff alignment
    movi.n   a7,   0xf                                  // 0xf alignment mask (16-byte alignment)
    and     a15,   a7,  a3                              // 16-byte alignment mask AND dest_buff pointer
    bnez    a15,   _unaligned_by_4byte                  // branch if a15 not equals to zero

    // Check dest_stride alignment
    and     a15,   a7,  a6                              // 16-byte alignment mask AND dest_stride
    bnez    a15,   _unaligned_by_4byte                  // branch if a15 not equals to zero

    // Check dest_w_bytes alignment
    and     a15,   a7,  a11                             // 16-byte alignment mask AND dest_w_bytes
    bnez    a15,   _unaligned_by_4byte                  // branch if a15 not equals to zero

//**********************************************************************************************************************

    // all aligned, the most ideal case

    // dest_buff   (a3) - 16-byte aligned
    // dest_stride (a6) - 16-byte multiple
    // dest_w      (a4) - 16-byte multiple

    srli    a9,    a4,   3                              // a9 - loop_len = dest_w / 8
    sub     a6,    a6,   a11                            // dest_stride = dest_stride - dest_w_bytes

    .outer_loop_aligned:

        loopnez  a9, ._main_loop_aligned                // 16 bytes (8 rgb565) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_aligned:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return

    _unaligned_by_4byte:

    // Check dest_buff alignment
    movi.n   a7,    0x3                                 // 0x3 alignment mask (4-byte alignment)
    and     a15,    a7,   a3                            // 4-byte alignment mask AND dest_buff pointer
    bnez    a15,    _unaligned_by_1byte                 // branch if a15 not equals to zero

    // Check dest_stride alignment
    and     a15,    a7,   a6                            // 4-byte alignment mask AND dest_stride pointer
    bnez    a15,    _unaligned_by_1byte                 // branch if a15 not equals to zero

//**********************************************************************************************************************

    // either dest_buff or dest_stride is not 16-byte aligned
    // dest_w is always 4-byte multiple
    // all of the following are 4-byte aligned

    // dest_buff   (a3) - 16-byte, or 4-byte aligned
    // dest_stride (a6) - 16-byte, or 4-byte multiple
    // dest_w      (a4) - 4-byte multiple

    sub      a6,    a6,   a11                           // dest_stride = dest_stride - dest_w_bytes
    movi.n   a7,    0xf                                 // 0xf alignment mask

    .outer_loop_aligned_by_4byte:

        // alignment check
        and     a15,   a7,  a3                          // 0xf (alignment mask) AND dest_buff pointer
        mov     a12,   a11                              // a12 - local_dest_w_bytes = dest_w_bytes
        beqz    a15,   _dest_buff_aligned_by_4byte       // branch if a15 equals to zero


            movi.n  a14,   16                           // a14 - 16
            sub     a15,   a14,   a15                   // a15 = 16 - unalignment (lower 4 bits of dest_buff address)
            sub     a12,   a12,   a15                   // local_dest_w_bytes = len - (16 - unalignment)

            // keep setting until dest_buff is aligned
            // Check modulo 8 of the unalignment, if - then set 8 bytes
            bbci    a15,  3, _aligning_mod_8_check_4byte // branch if 3-rd bit of unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                s32i.n      a10,  a3,  4                // save 32 bits from a10 to dest_buff a3, offset 4 bytes
                addi.n      a3,   a3,  8                // increment dest_buff pointer by 8 bytes
            _aligning_mod_8_check_4byte:

            // Check modulo 4 of the unalignment, if - then set 4 bytes
            bbci a15, 2, _aligning_mod_4_check_4byte     // branch if 2-nd bit unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment dest_buff pointer by 4 bytes
            _aligning_mod_4_check_4byte:

        _dest_buff_aligned_by_4byte:
        // Calculate main loop_len
        srli    a9,    a12,   4                         // a9 - loop_len = local_dest_w_bytes / 16

        // Main loop
        loopnez  a9, ._main_loop_unaligned_by_4byte     // 16 bytes (8 rgb565) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_unaligned_by_4byte:

        // Check modulo 8 of the dest_w, if - then set 8 bytes
        bbci a12, 3, _aligned_mod_8_check_4byte         // branch if 3-rd bit of local_dest_w_bytes a12 is clear
            ee.vst.l.64.ip    q0,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
        _aligned_mod_8_check_4byte:

        // Check modulo 4 of the dest_w, if - then set 4 bytes
        bbci a12, 2, _aligned_mod_4_check_4byte         // branch if 2-nd bit of local_dest_w_bytes a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
        _aligned_mod_4_check_4byte:

        // Check modulo 2 of the dest_w, if - then set 2 bytes
        bbci a12, 1, _aligned_mod_2_check_4byte         // branch if 1-st bit of local_dest_w_bytes a12 is clear
            s16i        a10,  a3,  0                    // save 16 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
        _aligned_mod_2_check_4byte:


        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned_by_4byte

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return

    _unaligned_by_1byte:

//**********************************************************************************************************************

    // either dest_buff or dest_stride is not 4-byte aligned
    // dest_w is always 4-byte multiple

    // dest_buff   (a3) - 4-byte, or 1-byte aligned
    // dest_stride (a6) - 4-byte, or 1-byte multiple
    // dest_w      (a4) - 4-byte multiple


    ee.zero.q   q1                                      // clear q1 
    ee.orq      q1,    q1,   q0                         // copy q0 to q1
    sub         a6,    a6,   a11                        // dest_stride = dest_stride - dest_w_bytes
    movi.n      a7,    0xf                              // 0xf alignment mask

    .outer_loop_aligned_by_1byte:

        // alignment check
        and     a15,   a7,  a3                          // 0xf (alignment mask) AND dest_buff pointer
        mov     a12,   a11                              // a12 - local_dest_w_bytes = dest_w_bytes
        beqz    a15,   _dest_buff_aligned_by_1byte      // branch if a15 equals to zero


            movi.n  a14,   16                           // a14 - 16
            sub     a15,   a14,   a15                   // a15 = 16 - unalignment (lower 4 bits of dest_buff address)
            sub     a12,   a12,   a15                   // local_dest_w_bytes = len - (16 - unalignment)

            // keep setting until dest_buff is aligned
            // Check modulo 8 of the unalignment, if - then set 8 bytes
            bbci    a15,  3, _aligning_mod_8_check_1byte// branch if 3-rd bit of unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                s32i.n      a10,  a3,  4                // save 32 bits from a10 to dest_buff a3, offset 4 bytes
                addi.n      a3,   a3,  8                // increment dest_buff pointer by 8 bytes
            _aligning_mod_8_check_1byte:

            // Check modulo 4 of the unalignment, if - then set 4 bytes
            bbci a15, 2, _aligning_mod_4_check_1byte    // branch if 2-nd bit unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment dest_buff pointer by 4 bytes
            _aligning_mod_4_check_1byte:

            // Check modulo 2 and 1
            // modulo 2 and modulo 1 requires the same action
            bbci a15, 1, _aligning_mod_2_check_1byte    // branch if 1-st bit unalignment a15 is clear
                s16i        a10,  a3,  0                // save 16 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  2                // increment dest_buff pointer by 2 bytes
            _aligning_mod_2_check_1byte:

            bbci a15, 0, _dest_buff_aligned_by_1byte    // branch if 0-st bit unalignment a15 is clear
                s16i        a10,  a3,  0                // save 16 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  2                // increment dest_buff pointer by 2 bytes
        _dest_buff_aligned_by_1byte:

        // Shift q reg, allowing to set 16-byte unaligned adata
        wur.sar_byte     a15                            // apply unalignment to the SAR_BYTE
        ee.src.q   q2,  q0,  q1                         // shift concat. of q0 and q1 to q2 by SAR_BYTE amount

        // Calculate main loop_len
        srli    a9,    a12,   4                         // a9 - loop_len = local_dest_w_bytes / 16

        // Main loop
        loopnez  a9, ._main_loop_unaligned_by_1byte     // 16 bytes (8 rgb565) in one loop
            ee.vst.128.ip q2, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_unaligned_by_1byte:

        // Firstly check mod 1 and mod 2 - correcting the aligned memory access
        // Go back in one Byte, allow to correct after ee.vst.128.ip aligned access
        addi    a3, a3, -4

        // Check modulo 2 of the dest_w, if - then set 2 bytes
        // set SSSS in 0xSSSS0000
        bbci a12, 1, _aligned_mod_2_check_1byte_corr    // branch if 1-st bit of dest_w a12 is clear
            srli    a14,   a10,  16                     // shift a10 in 16, allowing s16i (saving of lower 16 bits)
            s16i    a14,   a3,   2                      // save 16 bits from a10 to dest_buff a3, offset 2 bytes

            // Check modulo 1 of the dest_w, if - then set 1 byte
            // additionally set SS in 0x0000SS00
            bbci a12, 0, _aligned_end                   // branch if 0-th bit of dest_w a12 is clear
                srli    a14,   a10,  8                  // shift a10 in 8, allowing s8i
                s8i     a14,   a3,   1                  // save 8 bits from a10 to dest_buff a3, offset 1 byte
                j _aligned_end
        _aligned_mod_2_check_1byte_corr:

        // Check modulo 1 of the dest_w, if - then set 1 byte
        // set SS in 0xSS000000
        bbci a12, 0, _aligned_end                       // branch if 0-th bit of dest_w a12 is clear
            srli    a14,   a10,  24                     // shift a10 in 24, allowing s8i (saving of lower 8 bits)
            s8i     a14,   a3,   3                      // save 8 bits from a10 to dest_buff a3, offset 3 bytes
        _aligned_end:

        addi    a3, a3, 4                               // Increase the pointer back, correction for addi    a3, a3, -4

        // Check modulo 8 of the dest_w, if - then set 8 bytes
        bbci a12, 3, _aligned_mod_8_check_1byte         // branch if 3-rd bit of local_dest_w_bytes a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            s32i.n      a10,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  8                    // increment dest_buff pointer by 4 bytes
        _aligned_mod_8_check_1byte:

        // Check modulo 4 of the dest_w, if - then set 4 bytes
        bbci a12, 2, _aligned_mod_4_check_1byte         // branch if 2-nd bit of local_dest_w_bytes a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
        _aligned_mod_4_check_1byte:

        // Check modulo 2 of the dest_w, if - then set 2 bytes
        bbci a12, 1, _aligned_mod_2_check_1byte         // branch if 1-st bit of local_dest_w_bytes a12 is clear
            s16i        a10,  a3,  0                    // save 16 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
        _aligned_mod_2_check_1byte:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned_by_1byte

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return

    .lv_color_blend_to_rgb565_esp32_body:

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
