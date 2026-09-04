// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// <elf.h> is not guaranteed to exist under the ESP32 newlib toolchain
#define ELF_CLASS_32 1
#define ELF_CLASS_64 2
#define ELF_DATA_2LSB 1
#define ELF_TYPE_DYN 3
#define ELF_MACHINE_XTENSA 94
#define ELF_MACHINE_RISCV 243
#define ELF_MACHINE_X86_64 62
#define ELF_MACHINE_AARCH64 183

/** What a loader needs an ELF file's header to say before it will try to load it. */
struct ElfRequirements {
    uint8_t elf_class; /**< ELF_CLASS_32 / ELF_CLASS_64 */
    uint8_t data; /**< ELF_DATA_2LSB */
    uint16_t type; /**< ELF_TYPE_DYN */
    uint16_t machine; /**< ELF_MACHINE_XTENSA / _RISCV / _X86_64 / _AARCH64 */
};

/**
 * Reads the first 20 bytes of @a path (e_ident, e_type, e_machine; identical offsets for
 * ELF32 and ELF64) and checks the magic number and every field in @a requirements match.
 * @return false if @a path can't be opened, is too short, or doesn't match
 */
bool elf_check_file(const char* path, const struct ElfRequirements* requirements);

#ifdef __cplusplus
}
#endif
