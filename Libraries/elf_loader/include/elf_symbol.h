/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "private/elf_types.h"
#include "elf_symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ELFSYM_EXPORT(_sym)     { #_sym, (void*)&_sym }
#define ESP_ELFSYM_END              { NULL,  NULL }

/** @brief Function symbol description */

struct esp_elfsym {
    const char  *name;      /*!< Function name */
    const void  *sym;       /*!< Function pointer */
};

/**
 * @brief Find symbol address by name.
 *
 * @param sym_name - Symbol name
 * 
 * @return Symbol address if success or 0 if failed.
 */
uintptr_t elf_find_sym(const char *sym_name);

#ifdef CONFIG_ELF_LOADER_CUSTOMER_SYMBOLS
void elf_set_custom_symbols(const struct esp_elfsym* symbols);
#endif

#ifdef __cplusplus
}
#endif
