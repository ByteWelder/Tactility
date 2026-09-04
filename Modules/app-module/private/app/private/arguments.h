// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstring>
#include <new>

/**
 * Deep-copies @a argv (@a argc <= 0 => NULL, matching "no parameters"). Caller passes the result
 * to app_scheduler_start(), which takes ownership regardless of outcome.
 * @return NULL if @a argc <= 0 (no parameters), or if allocation failed. For @a argc > 0,
 * these are the only cases that produce NULL, so a caller can tell them apart by its own
 * already-known @a argc: NULL back from a positive @a argc always means allocation failed.
 * All partial allocations are freed before returning NULL, so failure never leaks memory.
 */
inline char** app_arguments_copy(int argc, const char* const argv[]) {
    if (argc <= 0) {
        return nullptr;
    }

    auto* copy = new (std::nothrow) char*[argc + 1];
    if (copy == nullptr) {
        return nullptr;
    }

    int copied = 0;
    for (; copied < argc; copied++) {
        size_t length = strlen(argv[copied]);
        copy[copied] = new (std::nothrow) char[length + 1];
        if (copy[copied] == nullptr) {
            break;
        }
        memcpy(copy[copied], argv[copied], length + 1);
    }

    if (copied < argc) {
        for (int i = 0; i < copied; i++) {
            delete[] copy[i];
        }
        delete[] copy;
        return nullptr;
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
