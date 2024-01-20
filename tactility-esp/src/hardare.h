#pragma once

#include "display.h"
#include "touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DisplayDevice* _Nonnull display;
    TouchDevice* _Nullable touch;
} Hardware;

#ifdef __cplusplus
}
#endif
