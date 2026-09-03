// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers app-module's default executable directories (data/bin, system/bin, the app install
 * root) with app_exec_path_add(). Called once from app-module's own Module::start().
 */
void app_exec_register_default_paths(void);

#ifdef __cplusplus
}
#endif
