/**
 * @brief key-value storage for general purpose.
 * Maps strings on a fixed set of data types.
 */
#pragma once

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* Bundle;

Bundle tt_bundle_alloc();
Bundle tt_bundle_alloc_copy(Bundle source);
void tt_bundle_free(Bundle bundle);

bool tt_bundle_get_bool(Bundle bundle, const char* key);
int tt_bundle_get_int(Bundle bundle, const char* key);
const char* tt_bundle_get_string(Bundle bundle, const char* key);

bool tt_bundle_opt_bool(Bundle bundle, const char* key, bool* out);
bool tt_bundle_opt_int(Bundle bundle, const char* key, int* out);
bool tt_bundle_opt_string(Bundle bundle, const char* key, char** out);

void tt_bundle_put_bool(Bundle bundle, const char* key, bool value);
void tt_bundle_put_int(Bundle bundle, const char* key, int value);
void tt_bundle_put_string(Bundle bundle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif
