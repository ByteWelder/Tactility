#include "tt_preferences.h"
#include <Tactility/Preferences.h>
#include <cstring>

#define HANDLE_AS_PREFERENCES(handle) ((tt::Preferences*)(handle))

extern "C" {

PreferencesHandle tt_preferences_alloc(const char* identifier) {
    return new tt::Preferences(identifier);
}

void tt_preferences_free(PreferencesHandle handle) {
    delete HANDLE_AS_PREFERENCES(handle);
}

bool tt_preferences_opt_bool(PreferencesHandle handle, const char* key, bool* out) {
    return HANDLE_AS_PREFERENCES(handle)->optBool(key, *out);
}

bool tt_preferences_opt_int32(PreferencesHandle handle, const char* key, int32_t* out) {
    return HANDLE_AS_PREFERENCES(handle)->optInt32(key, *out);
}
bool tt_preferences_opt_string(PreferencesHandle handle, const char* key, char* out, uint32_t outSize) {
    std::string out_string;

    if (!HANDLE_AS_PREFERENCES(handle)->optString(key, out_string)) {
        return false;
    }

    if (out_string.length() >= outSize) {
        // Need 1 byte to add 0 at the end
        return false;
    }

    memcpy(out, out_string.c_str(), out_string.length());
    out[out_string.length()] = 0x00;
    return true;
}

void tt_preferences_put_bool(PreferencesHandle handle, const char* key, bool value) {
    HANDLE_AS_PREFERENCES(handle)->putBool(key, value);
}

void tt_preferences_put_int32(PreferencesHandle handle, const char* key, int32_t value) {
    HANDLE_AS_PREFERENCES(handle)->putInt32(key, value);
}

void tt_preferences_put_string(PreferencesHandle handle, const char* key, const char* value) {
    HANDLE_AS_PREFERENCES(handle)->putString(key, value);
}

}