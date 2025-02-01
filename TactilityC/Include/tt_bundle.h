#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** The handle that represents a bundle instance */
typedef void* BundleHandle;

/** @return a new bundle instance */
BundleHandle tt_bundle_alloc();

/** Dealloc an existing bundle instance */
void tt_bundle_free(BundleHandle handle);

/**
 * Try to get a boolean value from a Bundle
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the output value (only set when return value is set to true)
 * @return true if "out" was set
 */
bool tt_bundle_opt_bool(BundleHandle handle, const char* key, bool* out);

/**
 * Try to get an int32_t value from a Bundle
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the output value (only set when return value is set to true)
 * @return true if "out" was set
 */
bool tt_bundle_opt_int32(BundleHandle handle, const char* key, int32_t* out);

/**
 * Try to get a string from a Bundle
 * @warning outSize must be large enough to include null terminator. This means that your string has to be the expected text length + 1 extra character.
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[out] out the buffer to store the string in
 * @param[in] outSize the size of the buffer
 * @return true if "out" was set
 */
bool tt_bundle_opt_string(BundleHandle handle, const char* key, char* out, uint32_t outSize);

/**
 * Store a boolean value in a Bundle
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_bundle_put_bool(BundleHandle handle, const char* key, bool value);

/**
 * Store an int32_t value in a Bundle
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_bundle_put_int32(BundleHandle handle, const char* key, int32_t value);

/**
 * Store a string value in a Bundle
 * @param[in] handle the handle that represents the bundle
 * @param[in] key the identifier that represents the stored value (~variable name)
 * @param[in] value the value to store
 */
void tt_bundle_put_string(BundleHandle handle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif