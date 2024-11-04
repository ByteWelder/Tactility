#ifndef ESP_PLATFOM

#include "bundle.h"
#include "preferences.h"
#include <string.h>
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
static void get_bundle_key(const char* namespace, const char* key, char* out) {
    strcpy(out, namespace);
    size_t namespace_len = strlen(namespace);
    out[namespace_len] = ':';
    char* out_with_key_offset = &out[namespace_len + 1];
    strcpy(out_with_key_offset, key);
}

static bool has_bool(const char* namespace, const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_has_bool(get_preferences_bundle(), bundle_key);
}

static bool has_int32(const char* namespace, const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_has_int32(get_preferences_bundle(), bundle_key);
}

static bool has_string(const char* namespace, const char* key) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_has_string(get_preferences_bundle(), bundle_key);
}

static bool opt_bool(const char* namespace, const char* key, bool* out) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_opt_bool(get_preferences_bundle(), bundle_key, out);
}

static bool opt_int32(const char* namespace, const char* key, int32_t* out) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_opt_int32(get_preferences_bundle(), bundle_key, out);
}

static bool opt_string(const char* namespace, const char* key, char* out, size_t* size) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    char* bundle_out = NULL;
    if (tt_bundle_opt_string(get_preferences_bundle(), bundle_key, &bundle_out)) {
        tt_assert(bundle_out != NULL);
        size_t found_length = strlen(bundle_out);
        tt_check(found_length <= (*size + 1), "output buffer not large enough");
        *size = found_length;
        strcpy(out, bundle_out);
        return true;
    } else {
        return false;
    }
}

static void put_bool(const char* namespace, const char* key, bool value) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_put_bool(get_preferences_bundle(), bundle_key, value);
}

static void put_int32(const char* namespace, const char* key, int32_t value) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_put_int32(get_preferences_bundle(), bundle_key, value);
}

static void put_string(const char* namespace, const char* key, const char* text) {
    char bundle_key[128];
    get_bundle_key(namespace, key, bundle_key);
    return tt_bundle_put_string(get_preferences_bundle(), bundle_key, text);
}

const Preferences preferences_memory = {
    .has_bool = &has_bool,
    .has_int32 = &has_int32,
    .has_string = &has_string,
    .opt_bool = &opt_bool,
    .opt_int32 = &opt_int32,
    .opt_string = &opt_string,
    .put_bool = &put_bool,
    .put_int32 = &put_int32,
    .put_string = &put_string
};

#endif