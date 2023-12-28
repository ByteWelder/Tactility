#include "applications_i.h"

// System services
extern const App desktop_app;
extern const App gui_app;
extern const App loader_app;

// System apps
extern const App system_info_app;

const App* const NANOBAKE_SERVICES[] = {
    &desktop_app,
    &gui_app,
    &loader_app
};

const size_t NANOBAKE_SERVICES_COUNT = sizeof(NANOBAKE_SERVICES) / sizeof(App*);

const App* const NANOBAKE_SYSTEM_APPS[] = {
    &system_info_app
};

const size_t NANOBAKE_SYSTEM_APPS_COUNT = sizeof(NANOBAKE_SYSTEM_APPS) / sizeof(App*);

const OnSystemStart NANOBAKE_ON_SYSTEM_START[] = {
};

const size_t NANOBAKE_ON_SYSTEM_START_COUNT = sizeof(NANOBAKE_ON_SYSTEM_START) / sizeof(OnSystemStart);
