#pragma once

#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct nb_hardware nb_hardware_t;

typedef void (*nb_on_system_start_)(nb_hardware_t* hardware);

extern const nb_app_t* const FLIPPER_SERVICES[];
extern const size_t FLIPPER_SERVICES_COUNT;

extern const nb_app_t* const FLIPPER_SYSTEM_APPS[];
extern const size_t FLIPPER_SYSTEM_APPS_COUNT;

extern const nb_on_system_start_ FLIPPER_ON_SYSTEM_START[];
extern const size_t FLIPPER_ON_SYSTEM_START_COUNT;

#ifdef __cplusplus
}
#endif
