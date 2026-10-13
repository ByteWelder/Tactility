// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * On the simulator, an app binary binds to xTaskCreate() and vTaskDelete() through the dynamic linker,
 * so these are the simulator's own definitions, built into FreeRTOS-Kernel in place of its own.
 * They call the hooks when set, passing the address they were called from.
 */

typedef BaseType_t (*FreeRtosTaskCreateHook)(const void* caller, TaskFunction_t function, const char* name, configSTACK_DEPTH_TYPE stack_depth, void* parameter, UBaseType_t priority, TaskHandle_t* out_handle);
typedef void (*FreeRtosTaskDeleteHook)(const void* caller, TaskHandle_t handle);

void freertos_set_task_hooks(FreeRtosTaskCreateHook create, FreeRtosTaskDeleteHook delete_task);

/** FreeRTOS-Kernel's own xTaskCreate() */
BaseType_t freertos_real_xTaskCreate(TaskFunction_t function, const char* name, configSTACK_DEPTH_TYPE stack_depth, void* parameter, UBaseType_t priority, TaskHandle_t* out_handle);

/** FreeRTOS-Kernel's own vTaskDelete() */
void freertos_real_vTaskDelete(TaskHandle_t handle);

/**
 * The POSIX port's own vPortCancelThread(), which joins the thread of a deleted task.
 * The simulator's vPortCancelThread() calls it with all signals blocked: glibc frees the joined thread's stack
 * while holding its stack cache lock. When the tick signal switches tasks while that lock is held,
 * the next pthread_create() blocks forever, because the port calls it with the tick signal blocked.
 */
void freertos_real_vPortCancelThread(void* task);

#ifdef __cplusplus
}
#endif
