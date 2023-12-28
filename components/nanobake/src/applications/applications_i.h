#pragma once

#include "app.h"
#include "devices.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OnSystemStart)(Devices* hardware);

extern const App* const NANOBAKE_SERVICES[];
extern const size_t NANOBAKE_SERVICES_COUNT;

extern const App* const NANOBAKE_SYSTEM_APPS[];
extern const size_t NANOBAKE_SYSTEM_APPS_COUNT;

extern const OnSystemStart NANOBAKE_ON_SYSTEM_START[];
extern const size_t NANOBAKE_ON_SYSTEM_START_COUNT;

#ifdef __cplusplus
}
#endif
