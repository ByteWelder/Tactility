#pragma once

#include "nb_display.h"
#include "nb_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    NbDisplay* _Nonnull display;
    NbTouch* _Nullable touch;
} NbHardware;

#ifdef __cplusplus
}
#endif
