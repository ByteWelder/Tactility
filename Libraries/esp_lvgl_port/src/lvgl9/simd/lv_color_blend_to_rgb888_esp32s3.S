/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL RGB888 simple fill for ESP32S3 processor

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
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = 2 * dest_w
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

    sub      a6,    a6,    a11                   // dest_stride = dest_stride - dest_w_bytes

    // Check for short lengths
    // dest_w should be at least 12, othewise it's not worth using esp32s3 TIE
    bgei     a4,   12,  _esp32s3_implementation         // Branch if dest_w is greater than or equal to 12
    j .lv_color_blend_to_rgb888_esp32_body              // Jump to esp32 implementation

    _esp32s3_implementation:

    // Prepare q registers for the main loop
    ee.movi.32.q   q3,   a13,  0                        // fill q3 register from a13 by 32 bits
    ee.movi.32.q   q3,   a14,  1                        // fill q3 register from a14 by 32 bits
    ee.movi.32.q   q3,   a15,  2                        // fill q3 register from a15 by 32 bits
    ee.movi.32.q   q3,   a13,  3                        // fill q3 register from a13 by 32 bits

    ee.movi.32.q   q4,   a14,  0                        // fill q4 register from a14 by 32 bits
    ee.movi.32.q   q4,   a15,  1                        // fill q4 register from a15 by 32 bits
    ee.movi.32.q   q4,   a13,  2                        // fill q4 register from a13 by 32 bits
    ee.movi.32.q   q4,   a14,  3                        // fill q4 register from a14 by 32 bits

    ee.movi.32.q   q5,   a15,  0                        // fill q5 register from a15 by 32 bits
    ee.movi.32.q   q5,   a13,  1                        // fill q5 register from a13 by 32 bits
    ee.movi.32.q   q5,   a14,  2                        // fill q5 register from a14 by 32 bits
    ee.movi.32.q   q5,   a15,  3                        // fill q5 register from a15 by 32 bits

    .outer_loop_aligned:

        // q registers will get shifted and clobbered, need to reinitialize them before using them again
        // Clear q registers
        ee.zero.q      q0                                   // clear q0
        ee.zero.q      q1                                   // clear q1
        ee.zero.q      q2                                   // clear q2

        // Reinitialize q registers
        ee.orq         q0,   q0,   q3                       // copy q3 to q0
        ee.orq         q1,   q1,   q4                       // copy q4 to q1
        ee.orq         q2,   q2,   q5                       // copy q5 to q2

        // alignment check
        extui   a8,    a3,  0,  4                           // address_alignment (a8) = dest_buff address (a3) AND 0xf

        movi.n  a12,   16                           // a12 = 16
        mov.n   a2,    a8                           // unalignment (a2) = a8
        // following instruction is here to avoid branching
        // need to adjust a8 == 0 to 16 to make the unalignment computation work
        moveqz  a2,    a12,   a8                    // modified unalignment (a2) = 16 if unalignment (a8) == 0

        sub     a2,    a12,   a2                    // a2  = 16 - unalignment (lower 4 bits of dest_buff address)
        sub     a10,   a11,   a2                    // local_dest_w_bytes = len - (16 - unalignment)

        movi.n  a12,   48                           // a12 = 48 (main loop copies 48 bytes)
        quou    a9,    a10,   a12                   // main_loop counter (a9) = local_dest_w_bytes (a10) DIV 48 (a12)
        remu    a10,   a10,   a12                   // a10 = local_dest_w_bytes (a10) MOD 48 (a12)

        beqz    a8,    _dest_buff_aligned           // If already aligned, skip aligning

        movi.n  a7,    unalignment_table            // Load unalignment_table address

        addx4   a7,    a8,    a7                    // jump_table handle (a7) = offset (a8) * 4 + jump_table address (a7)
        l32i    a7,    a7,    0                     // Load target address from jump table
        jx      a7                                  // Jump to the corresponding handler


// a13 - 0xBBRRGGBB a14 - 0xGGBBRRGG a15 - 0xRRGGBBRR
handle_0:
handle_1:
    s8i         a13,  a3,  0                    // save 8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s16i        a14,  a3,  0                    // save 16 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    s32i        a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    ee.vst.l.64.ip    q1,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_2:
    s16i        a13,  a3,  0                    // save 16 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    s32i        a15,  a3,  0                    // save 32 bits from a15 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    ee.vst.l.64.ip    q0,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_3:
    s8i         a13,  a3,  0                    // save 8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s32i        a14,  a3,  0                    // save 32 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    ee.vst.l.64.ip    q2,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_4:
    s32i        a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    ee.vst.l.64.ip    q1,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_5:
    s8i         a13,  a3,  0                    // save 8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s16i        a14,  a3,  0                    // save 16 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    ee.vst.l.64.ip    q0,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_6:
    s16i        a13,  a3,  0                    // save 16 bits from a13 to dest_buff a3, offset 0 byte
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    ee.vst.l.64.ip    q2,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_7:
    s8i         a13,  a3,  0                    // save 8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    ee.vst.l.64.ip    q1,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs
handle_8:
    ee.vst.l.64.ip    q0,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
    j _shift_q_regs

handle_9:
    s8i         a13,  a3,  0                    // save  8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s16i        a14,  a3,  0                    // save 16 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    s32i        a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    j _shift_q_regs
handle_10:
    s16i        a13,  a3,  0                    // save 16 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    s32i        a15,  a3,  0                    // save 32 bits from a15 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    j _shift_q_regs
handle_11:
    s8i         a13,  a3,  0                    // save  8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s32i        a14,  a3,  0                    // save 32 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    j _shift_q_regs
handle_12:
    s32i        a13,  a3,  0                    // save 32 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
    j _shift_q_regs
handle_13:
    s8i         a13,  a3,  0                    // save  8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    s16i        a14,  a3,  0                    // save 16 bits from a14 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    j _shift_q_regs
handle_14:
    s16i        a13,  a3,  0                    // save 16 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  2                    // increment dest_buff pointer by 2 bytes
    j _shift_q_regs
handle_15:
    s8i         a13,  a3,  0                    // save  8 bits from a13 to dest_buff a3, offset 0 bytes
    addi.n      a3,   a3,  1                    // increment dest_buff pointer by 1 byte
    j _shift_q_regs

.align 4

unalignment_table:
    .word handle_0            // Case 0: Dummy case for easier address computation
    .word handle_1            // Case 1: Align 15 bytes
    .word handle_2            // Case 2: Align 14 bytes
    .word handle_3            // Case 3: Align 13 bytes
    .word handle_4            // Case 4: Align 12 bytes
    .word handle_5            // Case 5: Align 11 bytes
    .word handle_6            // Case 6: Align 10 bytes
    .word handle_7            // Case 7: Align 9 bytes
    .word handle_8            // Case 8: Align 8 bytes
    .word handle_9            // Case 9: Align 7 bytes
    .word handle_10           // Case 10: Align 6 bytes
    .word handle_11           // Case 11: Align 5 bytes
    .word handle_12           // Case 12: Align 4 bytes
    .word handle_13           // Case 13: Align 3 bytes
    .word handle_14           // Case 14: Align 2 bytes
    .word handle_15           // Case 15: Align 1 byte


    _shift_q_regs:
        wur.sar_byte  a2                                // apply unalignment to the SAR_BYTE
        ee.src.q      q0,   q0,   q1                    // shift concat. of q0 and q1 to q0 by SAR_BYTE amount
        ee.src.q      q1,   q1,   q2                    // shift concat. of q1 and q2 to q1 by SAR_BYTE amount
        ee.src.q      q2,   q2,   q3                    // shift concat. of q2 and q3 to q2 by SAR_BYTE amount

    _dest_buff_aligned:
        loopnez a9, ._main_loop_aligned                 // 48 bytes (16 rgb888) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
            ee.vst.128.ip q1, a3, 16                    // store 16 bytes from q1 to dest_buff a3
            ee.vst.128.ip q2, a3, 16                    // store 16 bytes from q2 to dest_buff a3
        ._main_loop_aligned:

        // Check modulo 32 of the unalignment, if - then set 32 bytes
        bbci      a10,   5,  .lt_32                     // branch if 5-th bit of local_dest_w_bytes a10 is clear
            ee.vst.128.ip q0,  a3,  16                  // store 16 bytes from q0 to dest_buff a3
            ee.vst.128.ip q1,  a3,  16                  // store 16 bytes from q1 to dest_buff a3

            ee.srci.2q    q0,  q1,  1                   // shift q0 register to have next bytes to store ready from LSB
        .lt_32:

        // Check modulo 16 of the unalignment, if - then set 16 bytes
        bbci      a10,   4,  .lt_16                     // branch if 4-th bit of local_dest_w_bytes a10 is clear
            ee.vst.128.ip q0,  a3,  16                  // store 16 bytes from q0 to dest_buff a3

            ee.srci.2q    q0,  q1,  0                   // shift q0 register to have next bytes to store ready from LSB
        .lt_16:

        // Check modulo 8 of the unalignment, if - then set 8 bytes
        bbci      a10,   3,  .lt_8
            ee.vst.l.64.ip q0, a3, 8                    // store 8 bytes from q0 to dest_buff a3

            ee.srci.2q    q0,  q1,  1                   // shift q0 register to have next bytes to store ready from LSB
        .lt_8:

        // Check modulo 4 of the unalignment, if - then set 4 bytes
        bbci      a10,   2,  .lt_4
            ee.movi.32.a  q0,  a2,  0                   // move lowest 32 bits of q0 to a2
            s32i.n        a2,  a3,  0                   // save 32 bits from a2 to dest_buff a3, offset 0
            addi.n        a3,  a3,  4                   // increment dest_buff pointer by 4 bytes

            ee.srci.2q    q0,  q1,  0                   // shift q0 register to have next bytes to store ready from LSB
        .lt_4:

        // Check modulo 2 of the unalignment, if - then set 2 bytes
        bbci      a10,   1,  .lt_2
            ee.movi.32.a  q0,  a2,  0                   // move lowest 32 bits of q0 to a2
            s16i          a2,  a3,  0                   // save 16 bits from a2 to dest_buff a3, offset 0
            addi.n        a3,  a3,  2                   // increment dest_buff pointer by 2 bytes

            ee.srci.2q    q0,  q1,  1                   // shift q0 register to have next bytes to store ready from LSB
        .lt_2:

        // Check modulo 1 of the unalignment, if - then set 1 byte
        bbci      a10,   0,  .lt_1
            ee.movi.32.a  q0,  a2,  0                    // move lowest 32 bits of q0 to a2
            s8i           a2,  a3,  0                    // save 8 bits from a2 to dest_buff a3, offset 0
            addi.n        a3,  a3,  1                    // increment dest_buff pointer by 1 byte
        .lt_1:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return

    .lv_color_blend_to_rgb888_esp32_body:

    // Prepare main loop length and dest_w_bytes
    srli     a9,     a4,    2                    // a9 = loop_len = dest_w / 4, calculate main loop_len for original dest_w
    movi.n   a8,     0x3                         // a8 = 0x3, remainder mask
    and      a10,    a4,    a8                   // a10 - remainder after division by 4 = a4 & 0x3

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
        and     a7,  a8,  a3                            // a7 = dest_buff AND 0x3 (chck if the address is 4-byte aligned)
    bnez a5, .outer_loop

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return
