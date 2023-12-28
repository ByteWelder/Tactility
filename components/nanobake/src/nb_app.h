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
} NbAppType;

typedef enum {
    NB_TASK_PRIORITY_DEFAULT = 10
} NbTaskPriority;

typedef enum {
    NB_TASK_STACK_SIZE_DEFAULT = 2048
} NbStackSize;

typedef int32_t (*NbAppEntryPoint)(void _Nonnull* parameter);

typedef struct {
    const char id[NB_APP_ID_LENGTH];
    const char name[NB_APP_NAME_LENGTH];
    const NbAppType type;
    const NbAppEntryPoint _Nullable entry_point;
    const NbStackSize stack_size;
    const NbTaskPriority priority;
} NbApp;

#ifdef __cplusplus
}
#endif
