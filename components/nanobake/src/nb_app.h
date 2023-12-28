#pragma once

#include "esp_err.h"
#include "lvgl.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NB_APP_ID_LENGTH 32
#define NB_APP_NAME_LENGTH 32

typedef enum {
    SERVICE,
    SYSTEM,
    USER
} AppType;

typedef enum {
    NB_TASK_PRIORITY_DEFAULT = 10
} AppPriority;

typedef enum {
    NB_TASK_STACK_SIZE_DEFAULT = 2048
} AppStackSize;

typedef int32_t (*AppEntryPoint)(void _Nonnull* parameter);

typedef struct {
    const char id[NB_APP_ID_LENGTH];
    const char name[NB_APP_NAME_LENGTH];
    const AppType type;
    const AppEntryPoint _Nullable entry_point;
    const AppStackSize stack_size;
    const AppPriority priority;
} App;

#ifdef __cplusplus
}
#endif
