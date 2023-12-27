#pragma once

#include "nb_display.h"
#include "nb_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_hardware nb_hardware_t;
struct nb_hardware {
    nb_display_t* _Nonnull display;
    nb_touch_t* _Nullable touch;
};

#ifdef __cplusplus
}
#endif
