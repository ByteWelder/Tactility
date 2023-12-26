#include "nb_applications.h"

// System services
extern const nb_app_t desktop_app;
extern const nb_app_t gui_app;
extern const nb_app_t loader_app;

// System apps
extern const nb_app_t system_info_app;

const nb_app_t* const FLIPPER_SERVICES[] = {
    &desktop_app,
    &gui_app,
    &loader_app
};

const size_t FLIPPER_SERVICES_COUNT = sizeof(FLIPPER_SERVICES) / sizeof(nb_app_t*);

const nb_app_t* const FLIPPER_SYSTEM_APPS[] = {
    &system_info_app
};

const size_t FLIPPER_SYSTEM_APPS_COUNT = sizeof(FLIPPER_SYSTEM_APPS) / sizeof(nb_app_t*);

const nb_on_system_start_ FLIPPER_ON_SYSTEM_START[] = {
};

const size_t FLIPPER_ON_SYSTEM_START_COUNT = sizeof(FLIPPER_ON_SYSTEM_START) / sizeof(nb_on_system_start_);
