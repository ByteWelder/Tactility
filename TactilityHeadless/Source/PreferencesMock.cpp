#ifndef ESP_PLATFOM

#include "Tactility/Preferences.h"

#include <Tactility/Bundle.h>

namespace tt {

static Bundle preferences;

/**
 * Creates a string that is effectively "namespace:key" so we can create a single map (bundle)
 * to store all the key/value pairs.
 *
 * @param[in] namespace
 * @param[in] key
 * @param[out] out
 */
std::string get_bundle_key(const std::string& namespace_, const std::string& key) {
    return namespace_ + ':' + key;
}

bool Preferences::hasBool(const std::string& key) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.hasBool(bundle_key);
}

bool Preferences::hasInt32(const std::string& key) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.hasInt32(bundle_key);
}

bool Preferences::hasString(const std::string& key) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.hasString(bundle_key);
}

bool Preferences::optBool(const std::string& key, bool& out) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.optBool(bundle_key, out);
}

bool Preferences::optInt32(const std::string& key, int32_t& out) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.optInt32(bundle_key, out);
}

bool Preferences::optString(const std::string& key, std::string& out) const {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.optString(bundle_key, out);
}

void Preferences::putBool(const std::string& key, bool value) {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.putBool(bundle_key, value);
}

void Preferences::putInt32(const std::string& key, int32_t value) {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.putInt32(bundle_key, value);
}

void Preferences::putString(const std::string& key, const std::string& value) {
    std::string bundle_key = get_bundle_key(namespace_, key);
    return preferences.putString(bundle_key, value);
}

#endif

} // namespace
