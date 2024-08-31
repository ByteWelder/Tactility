#include "preferences.h"

#ifdef ESP_PLATFORM
extern const Preferences preferences_esp;
#else
extern const Preferences preferences_memory;
#endif

const Preferences* tt_preferences() {
#ifdef ESP_PLATFORM
    return &preferences_esp;
#else
    return &preferences_memory;
#endif
}