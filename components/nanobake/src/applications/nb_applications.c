#include "nb_applications.h"

// System services
extern const App desktop_app;
extern const App gui_app;
extern const App loader_app;

// System apps
extern const App system_info_app;

const App* const FLIPPER_SERVICES[] = {
    &desktop_app,
    &gui_app,
    &loader_app
};

const size_t FLIPPER_SERVICES_COUNT = sizeof(FLIPPER_SERVICES) / sizeof(App*);

const App* const FLIPPER_SYSTEM_APPS[] = {
    &system_info_app
};

const size_t FLIPPER_SYSTEM_APPS_COUNT = sizeof(FLIPPER_SYSTEM_APPS) / sizeof(App*);

const OnSystemStart FLIPPER_ON_SYSTEM_START[] = {
};

const size_t FLIPPER_ON_SYSTEM_START_COUNT = sizeof(FLIPPER_ON_SYSTEM_START) / sizeof(OnSystemStart);
