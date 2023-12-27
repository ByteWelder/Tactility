#pragma once

#include <stdio.h>
#include <esp_err.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NB_APP_ID_LENGTH 32
#define NB_APP_NAME_LENGTH 32

typedef enum nb_app_type nb_app_type_t;

enum nb_app_type {
    SERVICE,
    SYSTEM,
    USER
};

typedef struct nb_app nb_app_t;

typedef int32_t (*nb_app_entry_point) (void _Nonnull* parameter);

struct nb_app {
    const char id[NB_APP_ID_LENGTH];
    const char name[NB_APP_NAME_LENGTH];
    const nb_app_type_t type;
    const nb_app_entry_point _Nullable entry_point;
    const size_t stack_size;
    const uint32_t priority;
};

#ifdef __cplusplus
}
#endif
