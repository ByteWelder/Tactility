#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AppTypeService,
    AppTypeSystem,
    AppTypeUser
} AppType;

typedef enum {
    AppStackSizeNormal = 2048
} AppStackSize;

typedef int32_t (*AppEntryPoint)(void _Nonnull* parameter);

typedef struct {
    const char* _Nonnull id;
    const char* _Nonnull name;
    const char* _Nullable icon;
    const AppType type;
    const AppEntryPoint _Nullable entry_point;
    const AppStackSize stack_size;
} AppManifest;

#ifdef __cplusplus
}
#endif
