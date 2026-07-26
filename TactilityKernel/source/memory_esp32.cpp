// SPDX-License-Identifier: Apache-2.0
#ifdef ESP_PLATFORM

#include <tactility/memory.h>

#include <esp_heap_caps.h>

namespace {

uint32_t toHeapCaps(uint16_t capabilityFlags) {
    uint32_t caps = 0;
    if (capabilityFlags & MEMORY_CAPABILITY_INTERNAL) caps |= MALLOC_CAP_INTERNAL;
    if (capabilityFlags & MEMORY_CAPABILITY_EXTERNAL) caps |= MALLOC_CAP_SPIRAM;
    if (capabilityFlags & MEMORY_CAPABILITY_EXECUTABLE) caps |= MALLOC_CAP_EXEC;
    if (capabilityFlags & MEMORY_CAPABILITY_DMA) caps |= MALLOC_CAP_DMA;
    if (capabilityFlags & MEMORY_CAPABILITY_SIMD) caps |= MALLOC_CAP_SIMD;
    return caps;
}

} // namespace

extern "C" {

void* memory_alloc_with_policy(size_t size, const struct MemoryPolicy* policy) {
    uint32_t required_caps = toHeapCaps(policy->required);
    uint32_t desired_caps = toHeapCaps(policy->desired);

    void* ptr;
    if (policy->alignment > 0) {
        ptr = heap_caps_aligned_alloc(policy->alignment, size, required_caps | desired_caps);
        if (ptr == nullptr && desired_caps != 0) {
            // Desired caps couldn't be satisfied alongside the required ones - retry with
            // required only, since desired is explicitly optional.
            ptr = heap_caps_aligned_alloc(policy->alignment, size, required_caps);
        }
    } else {
        ptr = heap_caps_malloc(size, required_caps | desired_caps);
        if (ptr == nullptr && desired_caps != 0) {
            ptr = heap_caps_malloc(size, required_caps);
        }
    }
    return ptr;
}

void* memory_realloc_with_policy(void* ptr, size_t size, const struct MemoryPolicy* policy) {
    uint32_t required_caps = toHeapCaps(policy->required);
    uint32_t desired_caps = toHeapCaps(policy->desired);

    // No aligned-realloc counterpart in the heap_caps API - policy->alignment is only honored
    // on fresh allocations (memory_alloc_with_policy/memory_calloc_with_policy).
    void* result = heap_caps_realloc(ptr, size, required_caps | desired_caps);
    if (result == nullptr && desired_caps != 0) {
        result = heap_caps_realloc(ptr, size, required_caps);
    }
    return result;
}

void* memory_calloc_with_policy(size_t count, size_t size, const struct MemoryPolicy* policy) {
    uint32_t required_caps = toHeapCaps(policy->required);
    uint32_t desired_caps = toHeapCaps(policy->desired);

    void* ptr;
    if (policy->alignment > 0) {
        ptr = heap_caps_aligned_calloc(policy->alignment, count, size, required_caps | desired_caps);
        if (ptr == nullptr && desired_caps != 0) {
            ptr = heap_caps_aligned_calloc(policy->alignment, count, size, required_caps);
        }
    } else {
        ptr = heap_caps_calloc(count, size, required_caps | desired_caps);
        if (ptr == nullptr && desired_caps != 0) {
            ptr = heap_caps_calloc(count, size, required_caps);
        }
    }
    return ptr;
}

void memory_free(void* ptr) {
    heap_caps_free(ptr);
}

} // extern "C"

#endif // ESP_PLATFORM
