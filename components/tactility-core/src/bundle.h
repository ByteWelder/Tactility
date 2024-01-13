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

Bundle bundle_alloc();
Bundle bundle_alloc_copy(Bundle source);
void bundle_free(Bundle bundle);

bool bundle_get_bool(Bundle bundle, const char* key);
int bundle_get_int(Bundle bundle, const char* key);
const char* bundle_get_string(Bundle bundle, const char* key);

bool bundle_opt_bool(Bundle bundle, const char* key, bool* out);
bool bundle_opt_int(Bundle bundle, const char* key, int* out);
bool bundle_opt_string(Bundle bundle, const char* key, char** out);

void bundle_put_bool(Bundle bundle, const char* key, bool value);
void bundle_put_int(Bundle bundle, const char* key, int value);
void bundle_put_string(Bundle bundle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif
