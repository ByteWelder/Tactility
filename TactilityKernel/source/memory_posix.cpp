// SPDX-License-Identifier: Apache-2.0
#ifndef ESP_PLATFORM

#include <tactility/memory.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

// posix_memalign requires a power-of-2 alignment that's at least sizeof(void*).
size_t normalizeAlignment(uint8_t alignment) {
    size_t result = alignment;
    if (result < sizeof(void*)) {
        result = sizeof(void*);
    }
    return result;
}

} // namespace

extern "C" {

// MEMORY_CAP_* flags are meaningless on the desktop simulator (no capability-restricted memory regions)
// policy->required/desired are intentionally ignored here.
void* memory_alloc_with_policy(size_t size, const struct MemoryPolicy* policy) {
    if (policy->alignment > 0) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, normalizeAlignment(policy->alignment), size) != 0) {
            return nullptr;
        }
        return ptr;
    }
    return malloc(size);
}

void* memory_realloc_with_policy(void* ptr, size_t size, const struct MemoryPolicy* policy) {
    // Alignment can't be preserved across a POSIX realloc; only honored on fresh allocations
    // (memory_alloc_with_policy/memory_calloc_with_policy).
    return realloc(ptr, size);
}

void* memory_calloc_with_policy(size_t count, size_t size, const struct MemoryPolicy* policy) {
    if (policy->alignment > 0) {
        size_t total_size = count * size;
        if (count != 0 && total_size / count != size) {
            // count * size overflowed - reject rather than under-allocating.
            return nullptr;
        }

        void* ptr = nullptr;
        if (posix_memalign(&ptr, normalizeAlignment(policy->alignment), total_size) != 0) {
            return nullptr;
        }
        memset(ptr, 0, total_size);
        return ptr;
    }
    return calloc(count, size);
}

void memory_free(void* ptr) {
    free(ptr);
}

} // extern "C"

#endif // !ESP_PLATFORM
