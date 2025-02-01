#include "tt_bundle.h"
#include <Tactility/Bundle.h>
#include <cstring>

#define HANDLE_AS_BUNDLE(handle) ((tt::Bundle*)(handle))

extern "C" {

BundleHandle tt_bundle_alloc() {
    return new tt::Bundle();
}

void tt_bundle_free(BundleHandle handle) {
    delete HANDLE_AS_BUNDLE(handle);
}

bool tt_bundle_opt_bool(BundleHandle handle, const char* key, bool* out) {
    return HANDLE_AS_BUNDLE(handle)->optBool(key, *out);
}

bool tt_bundle_opt_int32(BundleHandle handle, const char* key, int32_t* out) {
    return HANDLE_AS_BUNDLE(handle)->optInt32(key, *out);
}
bool tt_bundle_opt_string(BundleHandle handle, const char* key, char* out, uint32_t outSize) {
    std::string out_string;
    if (HANDLE_AS_BUNDLE(handle)->optString(key, out_string)) {
        if (out_string.length() < outSize) { // Need 1 byte to add 0 at the end
            memcpy(out, out_string.c_str(), out_string.length());
            out[out_string.length()] = 0x00;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void tt_bundle_put_bool(BundleHandle handle, const char* key, bool value) {
    HANDLE_AS_BUNDLE(handle)->putBool(key, value);
}

void tt_bundle_put_int32(BundleHandle handle, const char* key, int32_t value) {
    HANDLE_AS_BUNDLE(handle)->putInt32(key, value);
}

void tt_bundle_put_string(BundleHandle handle, const char* key, const char* value) {
    HANDLE_AS_BUNDLE(handle)->putString(key, value);
}

}