#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Note that on ESP32, there are limitations:
 * - namespace name is limited by NVS_NS_NAME_MAX_SIZE (generally 16 characters)
 * - key is limited by NVS_KEY_NAME_MAX_SIZE (generally 16 characters)
 */

/** The handle that represents a Preferences instance */
typedef void* PreferencesHandle;

/**
 * @param[in] identifier the name of the preferences. This determines the NVS namespace on ESP.
 * @return a new preferences instance
 */
PreferencesHandle tt_preferences_alloc(const char* identifier);

/** Dealloc an existing preferences instance */
void tt_preferences_free(PreferencesHandle handle);

/**
 * Try to get a boolean value
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the output value (only set when return value is set to true)
 * @return true if "out" was set
 */
bool tt_preferences_opt_bool(PreferencesHandle handle, const char* key, bool* out);

/**
 * Try to get an int32_t value
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the output value (only set when return value is set to true)
 * @return true if "out" was set
 */
bool tt_preferences_opt_int32(PreferencesHandle handle, const char* key, int32_t* out);

/**
 * Try to get a string
 * @warning outSize must be large enough to include null terminator. This means that your string has to be the expected text length + 1 extra character.
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the buffer to store the string in
 * @param[in] outSize the size of the buffer
 * @return true if "out" was set
 */
bool tt_preferences_opt_string(PreferencesHandle handle, const char* key, char* out, uint32_t outSize);

/**
 * Store a boolean value
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_preferences_put_bool(PreferencesHandle handle, const char* key, bool value);

/**
 * Store an int32_t value
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_preferences_put_int32(PreferencesHandle handle, const char* key, int32_t value);

/**
 * Store a string value
 * @param[in] handle the handle that represents the preferences
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_preferences_put_string(PreferencesHandle handle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif