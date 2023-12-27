#pragma once

#include <stdio.h>
#include "esp_err.h"
#include "lvgl.h"

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

typedef int32_t (*NbAppEntryPoint) (void _Nonnull* parameter);

typedef struct {
    const char id[NB_APP_ID_LENGTH];
    const char name[NB_APP_NAME_LENGTH];
    const NbAppType type;
    const NbAppEntryPoint _Nullable entry_point;
    const size_t stack_size;
    const uint32_t priority;
} NbApp;

#ifdef __cplusplus
}
#endif
