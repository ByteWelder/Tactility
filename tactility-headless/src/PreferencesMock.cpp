#ifndef ESP_PLATFOM

#include "bundle.h"
#include "preferences.h"
#include <cstring>
#include <tactility_core.h>

static Bundle preferences_bundle;

static Bundle get_preferences_bundle() {
    if (preferences_bundle == NULL) {
        preferences_bundle = tt_bundle_alloc();
    }
    return preferences_bundle;
}

/**
 * Creates a string that is effectively "namespace:key" so we can create a single map (bundle)
 * to store all the key/value pairs.
 *
 * @param[in] namespace
 * @param[in] key
 * @param[out] out
 */
void get_bundle_key(const char* namespace_, const char* key, char* out) {
    strcpy(out, namespace_);
    size_t namespace_len = strlen(namespace_);
    out[namespace_len] = ':';
    char* out_with_key_offset = &out[namespace_len + 1];
    strcpy(out_with_key_offset, key);
}

bool Preferences::hasBool(const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_has_bool(get_preferences_bundle(), bundle_key);
}

bool Preferences::hasInt32(const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_has_int32(get_preferences_bundle(), bundle_key);
}

bool Preferences::hasString(const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_has_string(get_preferences_bundle(), bundle_key);
}

bool Preferences::optBool(const char* key, bool* out) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_opt_bool(get_preferences_bundle(), bundle_key, out);
}

bool Preferences::optInt32(const char* key, int32_t* out) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_opt_int32(get_preferences_bundle(), bundle_key, out);
}

bool Preferences::optString(const char* key, char* out, size_t* size) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    char* bundle_out = NULL;
    if (tt_bundle_opt_string(get_preferences_bundle(), bundle_key, &bundle_out)) {
        tt_assert(bundle_out != nullptr);
        size_t found_length = strlen(bundle_out);
        tt_check(found_length <= (*size + 1), "output buffer not large enough");
        *size = found_length;
        strcpy(out, bundle_out);
        return true;
    } else {
        return false;
    }
}

void Preferences::putBool(const char* key, bool value) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_put_bool(get_preferences_bundle(), bundle_key, value);
}

void Preferences::putInt32(const char* key, int32_t value) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_put_int32(get_preferences_bundle(), bundle_key, value);
}

void Preferences::putString(const char* key, const char* text) {
    char bundle_key[128];
    get_bundle_key(namespace_, key, bundle_key);
    return tt_bundle_put_string(get_preferences_bundle(), bundle_key, text);
}

#endif