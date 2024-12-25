#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* BundleHandle;

BundleHandle tt_bundle_alloc();
void tt_bundle_free(BundleHandle handle);

bool tt_bundle_opt_bool(BundleHandle handle, const char* key, bool* out);
bool tt_bundle_opt_int32(BundleHandle handle, const char* key, int32_t* out);
/**
 * Note that outSize must be large enough to include null terminator.
 * This means that your string has to be the expected text length + 1 extra character.
 */
bool tt_bundle_opt_string(BundleHandle handle, const char* key, char* out, uint32_t outSize);

void tt_bundle_put_bool(BundleHandle handle, const char* key, bool value);
void tt_bundle_put_int32(BundleHandle handle, const char* key, int32_t value);
void tt_bundle_put_string(BundleHandle handle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif