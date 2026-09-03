// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstring>

/**
 * Deep-copies @a argv (@a argc <= 0 => NULL, matching "no parameters"). Caller passes the result
 * to app_scheduler_start(), which takes ownership regardless of outcome.
 */
inline char** app_arguments_copy(int argc, const char* const argv[]) {
    if (argc <= 0) {
        return nullptr;
    }
    auto* copy = new char*[argc + 1];
    for (int i = 0; i < argc; i++) {
        size_t length = strlen(argv[i]);
        copy[i] = new char[length + 1];
        memcpy(copy[i], argv[i], length + 1);
    }
    copy[argc] = nullptr;
    return copy;
}

/**
 * Frees a deep-copied argv previously built by app_arguments_copy(): each individually
 * heap-allocated string, then the array itself. Safe to call with count == 0 / values == nullptr
 * (no-op).
 */
inline void app_arguments_free(int count, char** values) {
    if (values == nullptr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        delete[] values[i];
    }
    delete[] values;
}
