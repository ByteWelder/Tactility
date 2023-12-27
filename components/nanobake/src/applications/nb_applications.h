#pragma once

#include "nb_app.h"
#include "nb_hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NbOnSystemStart)(NbHardware* hardware);

extern const NbApp* const FLIPPER_SERVICES[];
extern const size_t FLIPPER_SERVICES_COUNT;

extern const NbApp* const FLIPPER_SYSTEM_APPS[];
extern const size_t FLIPPER_SYSTEM_APPS_COUNT;

extern const NbOnSystemStart FLIPPER_ON_SYSTEM_START[];
extern const size_t FLIPPER_ON_SYSTEM_START_COUNT;

#ifdef __cplusplus
}
#endif
