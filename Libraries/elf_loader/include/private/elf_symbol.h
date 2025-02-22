/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tactility custom -->
#define ESP_ELFSYM_EXPORT(_sym)     { #_sym, (const void*)&_sym }
// <-- Tactility custom
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

// Tactility custom -->
void elf_set_custom_symbols(const struct esp_elfsym* symbols);
// <-- Tactility custom

#ifdef __cplusplus
}
#endif
