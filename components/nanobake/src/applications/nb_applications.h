#pragma once

#include "devices.h"
#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OnSystemStart)(Devices* hardware);

extern const App* const FLIPPER_SERVICES[];
extern const size_t FLIPPER_SERVICES_COUNT;

extern const App* const FLIPPER_SYSTEM_APPS[];
extern const size_t FLIPPER_SYSTEM_APPS_COUNT;

extern const OnSystemStart FLIPPER_ON_SYSTEM_START[];
extern const size_t FLIPPER_ON_SYSTEM_START_COUNT;

#ifdef __cplusplus
}
#endif
