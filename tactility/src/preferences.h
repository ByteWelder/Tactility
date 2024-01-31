#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*PreferencesHasBool)(const char* namespace, const char* key);
typedef bool (*PreferencesHasInt32)(const char* namespace, const char* key);
typedef bool (*PreferencesHasString)(const char* namespace, const char* key);

typedef bool (*PreferencesOptBool)(const char* namespace, const char* key, bool* out);
typedef bool (*PreferencesOptInt32)(const char* namespace, const char* key, int32_t* out);
typedef bool (*PreferencesOptString)(const char* namespace, const char* key, char* out, size_t* size);

typedef void (*PreferencesPutBool)(const char* namespace, const char* key, bool value);
typedef void (*PreferencesPutInt32)(const char* namespace, const char* key, int32_t value);
typedef void (*PreferencesPutString)(const char* namespace, const char* key, const char* value);

typedef struct {
    PreferencesHasBool has_bool;
    PreferencesHasInt32 has_int32;
    PreferencesHasString has_string;

    PreferencesOptBool opt_bool;
    PreferencesOptInt32 opt_int32;
    PreferencesOptString opt_string;

    PreferencesPutBool put_bool;
    PreferencesPutInt32 put_int32;
    PreferencesPutString put_string;
} Preferences;

/**
 * @return an instance of Preferences.
 */
const Preferences* tt_preferences();

#ifdef __cplusplus
}
#endif