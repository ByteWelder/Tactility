#pragma once

#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FlipperInternalOnStartHook)(void);

extern const nb_app_t* const FLIPPER_SERVICES[];
extern const size_t FLIPPER_SERVICES_COUNT;

extern const nb_app_t* const FLIPPER_SYSTEM_APPS[];
extern const size_t FLIPPER_SYSTEM_APPS_COUNT;

extern const FlipperInternalOnStartHook FLIPPER_ON_SYSTEM_START[];
extern const size_t FLIPPER_ON_SYSTEM_START_COUNT;

#ifdef __cplusplus
}
#endif
