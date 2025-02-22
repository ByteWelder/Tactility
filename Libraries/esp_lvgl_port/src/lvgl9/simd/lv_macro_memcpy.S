/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Memcpy macros for modulo checking
// After running the main loop, there is need to check remaining bytes to be copied out of the main loop
// Macros work with both, aligned and unaligned (4-byte boundary) memories
// but performance is significantly lower when using unaligned memory, because of the unaligned memory access exception


// Macro for checking modulo 8
 .macro macro_memcpy_mod_8 src_buf, dest_buf, condition, x1, x2, JUMP_TAG
    // Check modulo 8 of the \condition, if - then copy 8 bytes
    bbci \condition, 3, ._mod_8_check_\JUMP_TAG      // Branch if 3-rd bit of \condition is clear
        l32i.n      \x1,        \src_buf,   0        // Load 32 bits from \src_buff to \x1, offset 0
        l32i.n      \x2,        \src_buf,   4        // Load 32 bits from \src_buff to \x2, offset 4
        s32i.n      \x1,        \dest_buf,  0        // Save 32 bits from \x1 to \dest_buff, offset 0
        s32i.n      \x2,        \dest_buf,  4        // Save 32 bits from \x2 to \dest_buff, offset 4
        addi.n      \src_buf,   \src_buf,   8        // Increment \src_buff pointer by 8
        addi.n      \dest_buf,  \dest_buf,  8        // Increment \dest_buff pointer 8
    ._mod_8_check_\JUMP_TAG:
.endm // macro_memcpy_mod_8


// Macro for checking modulo 4
 .macro macro_memcpy_mod_4 src_buf, dest_buf, condition, x1, JUMP_TAG
    // Check modulo 4 of the \condition, if - then copy 4 bytes
    bbci \condition, 2, ._mod_4_check_\JUMP_TAG      // Branch if 2-nd bit of \condition is clear
        l32i.n      \x1,        \src_buf,   0        // Load 32 bits from \src_buff to \x1, offset 0
        addi.n      \src_buf,   \src_buf,   4        // Increment \src_buff pointer by 4
        s32i.n      \x1,        \dest_buf,  0        // Save 32 bits from \x1 to \dest_buff, offset 0
        addi.n      \dest_buf,  \dest_buf,  4        // Increment \dest_buff pointer 4
    ._mod_4_check_\JUMP_TAG:
.endm // macro_memcpy_mod_4


// Macro for checking modulo 2
 .macro macro_memcpy_mod_2 src_buf, dest_buf, condition, x1, JUMP_TAG
    // Check modulo 2 of the \condition, if - then copy 2 bytes
    bbci \condition, 1, ._mod_2_check_\JUMP_TAG      // Branch if 1-st bit of \condition is clear
        l16ui       \x1,        \src_buf,   0        // Load 16 bits from \src_buff to \x1, offset 0
        addi.n      \src_buf,   \src_buf,   2        // Increment \src_buff pointer by 2
        s16i        \x1,        \dest_buf,  0        // Save 16 bits from \x1 to \dest_buff, offset 0
        addi.n      \dest_buf,  \dest_buf,  2        // Increment \dest_buff pointer 2
    ._mod_2_check_\JUMP_TAG:
.endm // macro_memcpy_mod_2


// Macro for checking modulo 1
 .macro macro_memcpy_mod_1 src_buf, dest_buf, condition, x1, JUMP_TAG
    // Check modulo 1 of the \condition, if - then copy 1 byte
    bbci \condition, 0, ._mod_1_check_\JUMP_TAG      // Branch if 0-th bit of \condition is clear
        l8ui        \x1,        \src_buf,   0        // Load 8 bits from \src_buff to \x1, offset 0
        addi.n      \src_buf,   \src_buf,   1        // Increment \src_buff pointer by 1
        s8i         \x1,        \dest_buf,  0        // Save 8 bits from \x1 to \dest_buff, offset 0
        addi.n      \dest_buf,  \dest_buf,  1        // Increment \dest_buff pointer 1
    ._mod_1_check_\JUMP_TAG:
.endm // macro_memcpy_mod_1
